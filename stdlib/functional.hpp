#ifndef STD_FUNCTIONAL_HPP
#define STD_FUNCTIONAL_HPP

#include "memory.hpp"
#include "type_traits.hpp"

namespace std
{
	template <typename UnusedType>
	class function;

	template <typename ReturnType, typename... ArgumentTypes>
	class function <ReturnType (ArgumentTypes...)>
	{
	public:
		function() : mInvoker() {}
		
		template <typename FunctionT>
		function(FunctionT f)
			: mInvoker(new free_function_holder<FunctionT>(f))
		{
		}
		
		template <typename FunctionType, typename ClassType>
		function(FunctionType ClassType::* f)
			: mInvoker(new member_function_holder<FunctionType, ArgumentTypes ...>(f))
		{
		}
		
		ReturnType operator() (ArgumentTypes... args)
		{
			return mInvoker->invoke(args ...);
		}
		
		function(const function& other)
			: mInvoker(other.mInvoker->clone())
		{
		}
		
		function& operator= (const function& other)
		{
			mInvoker = other.mInvoker->clone();
		}
		
	private:
		class function_holder_base
		{
		public:
			function_holder_base() {}
			virtual ~function_holder_base(){}
			
			virtual ReturnType invoke(ArgumentTypes ... args) = 0;
			virtual std::shared_ptr<function_holder_base> clone() = 0;
			
		private:
			function_holder_base(const function_holder_base&);
			void operator= (const function_holder_base&);
		};
		
		typedef std::shared_ptr<function_holder_base> invoker_t;
		
		template <typename FunctionT>
		class free_function_holder : public function_holder_base
		{
		public:
			typedef free_function_holder<FunctionT> self_type;
			
			free_function_holder(FunctionT func)
				: function_holder_base(), mFunction(func) {}
			
			virtual ReturnType invoke(ArgumentTypes... args)
			{
				return mFunction(args...);
			}
			
			virtual invoker_t clone()
			{
				return invoker_t(new self_type(mFunction));
			}
			
		private:
			FunctionT mFunction;
		};
		
		template <typename FunctionType, typename ClassType, typename... RestArgumentTypes>
		class member_function_holder : public function_holder_base
		{
		public:
			typedef FunctionType ClassType::* member_function_signature_t;
			
			member_function_holder(member_function_signature_t f)
				: mFunction(f) {}
			
			virtual ReturnType invoke(ClassType obj, RestArgumentTypes ... restArgs)
			{
				return (obj.*mFunction)(restArgs...);
			}
			
			virtual invoker_t clone()
			{
				return invoker_t(new member_function_holder(mFunction));
			}
			
		private:
			member_function_signature_t mFunction;
		};
		
		invoker_t mInvoker;
	};
	
	//the Main std::bind movable replacement
	template <typename ...P>
	class bind
	{
	private:
		//A helper class for storage and stitching params together
		template <typename A, typename ...Args>
		struct BindHelper : public BindHelper<Args...>
		{
			A a_;
		 
			BindHelper(A&& a, Args&& ...args) :
				BindHelper<Args...>(std::forward<Args>(args)...),
				a_(std::forward<A>(a))
			{}
		 
			template <typename Func, typename ...InArgs>
			void call(Func& f, InArgs && ...args)
			{
				BindHelper<Args...>::call(f,
					std::forward(args)...,
					std::move(a_));
			}
		};
		 
		//The helpers terminal case
		template <typename A>
		struct BindHelper<A>
		{
			A a_;
			
			BindHelper(A&& a) :
				a_(std::forward<A>(a))
			{}
			
			template <typename Func, typename ...InArgs>
			void call(Func& f, InArgs && ...args)
			{
				f(std::forward<A>(args)...,
					std::forward<A>(a_));
			}
		};
		
		typedef void (*F)(P&&...);
	
		F func_;
		BindHelper<P...> help_;
	 
	public:
		bind(F func, P&& ...p) :
			func_(func),
			help_(std::forward<P>(p)...)
		{}
		
		bind(F func, P& ...p) :
			func_(func),
			help_(p...)
		{}
		 
		~bind() {}
		
		void operator() ()
		{
			help_.call(func_);
		}
	};
}

#endif
