#ifndef STD_REFERENCE_WRAPPER_HPP
#define STD_REFERENCE_WRAPPER_HPP

namespace std
{
	// std::reference_wrapper
	template <class T, class... Args>
	class reference_wrapper
	{
	public:
		typedef T type;
		
		reference_wrapper(T& __x) noexcept
			: _ref( addressof(__x) ) {}
		
		reference_wrapper(T&&) = delete; // do not bind to temporary objects
		
		reference_wrapper(const reference_wrapper<T>& __x) noexcept
			: _ref( __x._ref ) {}
		
		reference_wrapper& operator= (const reference_wrapper<T>& __x) noexcept
		{
			_ref = __x._ref; return *this;
		}
		
		operator T&() const noexcept
		{
			return *_ref;
		}
		
		T& get() const noexcept
		{
			return *_ref;
		}
		
		template <class... ArgTypes>
		typename result_of<T& (ArgTypes&&...)>::type
			operator() (ArgTypes&&... args) const
		{
			return (*_ref)(forward<ArgTypes>(args)...);
		}
		
	private:
		typename remove_reference<type>::type* _ref;
	};
	
	// std::ref
	template<class T>
	reference_wrapper<T> ref(T& t)
	{
		return reference_wrapper<T>(t);
	}
	
	template<class T>
	reference_wrapper<T> ref(reference_wrapper<T> t)
	{
		return ref(t.get());
	}
	
	template <class T>
	void ref(const T&&) = delete;
	
	// std::cref
	template<class T>
	reference_wrapper<const T> cref(const T& t)
	{
		return reference_wrapper<const T>(t);
	}
	
	template<class T>
	reference_wrapper<const T> cref(reference_wrapper<T> t)
	{
		return cref(t.get());
	}
	
	template <class T>
	void cref(const T&&) = delete;
	
}

#endif
