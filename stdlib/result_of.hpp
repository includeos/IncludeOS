#ifndef STD_RESULT_OF_HPP
#define STD_RESULT_OF_HPP

#include "forward.hpp"

namespace std
{
	// See:
	// http://en.cppreference.com/w/cpp/types/result_of
	
	template <class> struct result_of;
	
	namespace detail
	{
		template <class F, class... Args>
		inline auto INVOKE(F&& f, Args&&... args) ->
			decltype(forward<F>(f)(forward<Args>(args)...))
		{
			return forward<F>(f)(forward<Args>(args)...);
		}
		 
		template <class Base, class T, class Derived>
		inline auto INVOKE(T Base::*&& pmd, Derived&& ref) ->
			decltype(forward<Derived>(ref).*forward<T Base::*>(pmd))
		{
			return forward<Derived>(ref).*forward<T Base::*>(pmd);
		}
		 
		template <class PMD, class Pointer>
		inline auto INVOKE(PMD&& pmd, Pointer&& ptr) ->
			decltype((*forward<Pointer>(ptr)).*forward<PMD>(pmd))
		{
			return (*forward<Pointer>(ptr)).*forward<PMD>(pmd);
		}
		 
		template <class Base, class T, class Derived, class... Args>
		inline auto INVOKE(T Base::*&& pmf, Derived&& ref, Args&&... args) ->
			decltype((forward<Derived>(ref).*forward<T Base::*>(pmf))(forward<Args>(args)...))
		{
			return (forward<Derived>(ref).*forward<T Base::*>(pmf))(forward<Args>(args)...);
		}
		 
		template <class PMF, class Pointer, class... Args>
		inline auto INVOKE(PMF&& pmf, Pointer&& ptr, Args&&... args) ->
			decltype(((*forward<Pointer>(ptr)).*forward<PMF>(pmf))(forward<Args>(args)...))
		{
			return ((*forward<Pointer>(ptr)).*forward<PMF>(pmf))(forward<Args>(args)...);
		}
	} // namespace detail
	
	// C++11 implementation:
	template <class F, class... ArgTypes>
	struct result_of<F(ArgTypes...)> {
		using type = decltype(detail::INVOKE(
			declval<F>(), declval<ArgTypes>()...
		));
	};
	 
	// C++14 implementation:
	/*namespace detail
	{
	template <class F, class... ArgTypes>
	struct invokeable
	{
		template <typename U = F>
		static auto test(int) -> decltype(INVOKE(
			declval<U>(), declval<ArgTypes>()...
		), void(), true_type());
	 
		static auto test(...) -> false_type;
	 
		static constexpr bool value = decltype(test(0))::value;
	};
	 
	template <bool B, class F, class... ArgTypes>
	struct _result_of
	{
		using type = decltype(INVOKE(
			declval<F>(), declval<ArgTypes>()...
		));
	};
	 
	template <class F, class... ArgTypes>
	struct _result_of<false, F, ArgTypes...> { };
	} // namespace detail
	 
	template <class F, class... ArgTypes>
	struct result_of<F(ArgTypes...)> :
		detail::_result_of<detail::invokeable<F, ArgTypes...>::value, F, ArgTypes...>
	{};*/
}

#endif
