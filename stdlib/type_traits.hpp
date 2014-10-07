#ifndef STD_TYPE_TRAITS_HPP
#define STD_TYPE_TRAITS_HPP

namespace std
{
	// See:
	// http://en.cppreference.com/w/cpp/types/remove_reference
	
	template<class T> struct remove_reference      { typedef T type; };
	template<class T> struct remove_reference<T&>  { typedef T type; };
	template<class T> struct remove_reference<T&&> { typedef T type; };
	
	template<class T>
	using remove_reference_t = typename remove_reference<T>::type;
	
	// http://en.cppreference.com/w/cpp/types/integral_constant
	template<class T, T v>
	struct integral_constant
	{
		static constexpr T value = v;
		typedef T value_type;
		typedef integral_constant type;
		constexpr operator value_type()   const noexcept { return value; }
		constexpr value_type operator()() const noexcept { return value; }
	};
	
	typedef integral_constant<bool, true>  true_type;
	typedef integral_constant<bool, false> false_type;
	
	// http://en.cppreference.com/w/cpp/types/is_reference
	template <class T> struct is_reference      : false_type {};
	template <class T> struct is_reference<T&>  : true_type  {};
	template <class T> struct is_reference<T&&> : true_type  {};
	
	// http://en.cppreference.com/w/cpp/types/is_rvalue_reference
	template <class T> struct is_rvalue_reference      : false_type {};
	template <class T> struct is_rvalue_reference<T&&> : true_type  {};
	
	// http://en.cppreference.com/w/cpp/types/is_lvalue_reference
	template<class T> struct is_lvalue_reference     : false_type {};
	template<class T> struct is_lvalue_reference<T&> : true_type  {};
	
	// std::add_rvalue_reference
	template<typename T>
    struct add_rvalue_reference
    {
		typedef T&& type;
	};
	
	// std::declval
	template <typename T>
	typename add_rvalue_reference<T>::type declval();
	
	// http://en.cppreference.com/w/cpp/types/enable_if
	template<bool B, class T = void>
	struct enable_if {};
	
	template<class T>
	struct enable_if<true, T> { typedef T type; };
	
}

#include "forward.hpp"
#include "result_of.hpp"

#endif
