#ifndef STD_TYPE_TRAITS_HPP
#define STD_TYPE_TRAITS_HPP

namespace std
{
	typedef unsigned size_t;
	typedef size_t size_type;
	
	typedef decltype(nullptr) nullptr_t;
	
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
	
	// std::true_type, std::false_type
	typedef integral_constant<bool, true>  true_type;
	typedef integral_constant<bool, false> false_type;
	
	// std::is_same
	template<class T, class U>
	struct is_same : false_type {};
	
	template<class T>
	struct is_same<T, T> : true_type {};
	
	// http://en.cppreference.com/w/cpp/types/is_reference
	template <class T> struct is_reference      : false_type {};
	template <class T> struct is_reference<T&>  : true_type  {};
	template <class T> struct is_reference<T&&> : true_type  {};
	
	// http://en.cppreference.com/w/cpp/types/remove_reference
	template<class T> struct remove_reference      { typedef T type; };
	template<class T> struct remove_reference<T&>  { typedef T type; };
	template<class T> struct remove_reference<T&&> { typedef T type; };
	
	template<class T>
	using remove_reference_t = typename remove_reference<T>::type;
	
	// http://en.cppreference.com/w/cpp/types/is_rvalue_reference
	template <class T> struct is_rvalue_reference      : false_type {};
	template <class T> struct is_rvalue_reference<T&&> : true_type  {};
	
	// http://en.cppreference.com/w/cpp/types/is_lvalue_reference
	template<class T> struct is_lvalue_reference     : false_type {};
	template<class T> struct is_lvalue_reference<T&> : true_type  {};
	
	// std::remove_volatile
	template<typename T>
	struct remove_volatile
	{
		typedef T type;
	};
	
	template<typename T>
	struct remove_volatile<T volatile>
	{
		typedef T type;
	};
	
	// std::remove_const
	template<class T> struct remove_const          { typedef T type; };
	template<class T> struct remove_const<const T> { typedef T type; };
	
	// std::remove_cv
	// http://en.cppreference.com/w/cpp/types/remove_cv
	
	template<class T>
	struct remove_cv
	{
		typedef typename remove_volatile<typename remove_const<T>::type>::type type;
	};
	
	// std::remove_extent
	// http://en.cppreference.com/w/cpp/types/remove_extent
	
	template<class T>
	struct remove_extent { typedef T type; };
	 
	template<class T>
	struct remove_extent<T[]> { typedef T type; };
	 
	template<class T, size_t N>
	struct remove_extent<T[N]> { typedef T type;};
	
	
	// std::enable_if
	// http://en.cppreference.com/w/cpp/types/enable_if
	
	template<bool B, class T = void>
	struct enable_if {};
	
	template<class T>
	struct enable_if<true, T> { typedef T type; };
	
	// std::is_void
	template<class T>
	struct is_void : 
	std::integral_constant<bool,
		std::is_same<void, typename std::remove_cv<T>::type>
	::value> {};
	
	// std::is_function
	// http://en.cppreference.com/w/cpp/types/is_function
	
	// primary template
	template<class>
	struct is_function : false_type { };
	 
	// specialization for regular functions
	template<class Ret, class... Args>
	struct is_function<Ret(Args...)> : true_type {};
	 
	// specialization for variadic functions such as std::printf
	template<class Ret, class... Args>
	struct is_function<Ret(Args......)> : true_type {};
	 
	// specialization for function types that have cv-qualifiers
	template<class Ret, class... Args>
	struct is_function<Ret(Args...)const> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args...)volatile> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args...)const volatile> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args......)const> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args......)volatile> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args......)const volatile> : true_type {};
	 
	// specialization for function types that have ref-qualifiers
	template<class Ret, class... Args>
	struct is_function<Ret(Args...) &> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args...)const &> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args...)volatile &> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args...)const volatile &> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args......) &> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args......)const &> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args......)volatile &> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args......)const volatile &> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args...) &&> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args...)const &&> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args...)volatile &&> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args...)const volatile &&> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args......) &&> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args......)const &&> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args......)volatile &&> : true_type {};
	template<class Ret, class... Args>
	struct is_function<Ret(Args......)const volatile &&> : true_type {};
	
	// std::is_array
	template<class T>
	struct is_array : false_type {};
	
	template<class T>
	struct is_array<T[]> : true_type {};
	
	template<class T, size_t N>
	struct is_array<T[N]> : true_type {};
	
	// std::is_union
	template<class T>
	struct is_union :
	    public integral_constant<bool, __is_union(T)>
    { };
	
	// std::is_class
	namespace detail
	{
		template <class T> char test(int T::*);
		struct two { char c[2]; };
		template <class T> two test(...);
	}
	
	template <class T>
	struct is_class : 
		integral_constant<bool, 
			sizeof(detail::test<T>(0)) == 1
		&& !is_union<T>::value> {};
	
	// std::is_object
	/* TODO: is_class
	template<class T>
	struct is_object : integral_constant<bool,
                     is_scalar<T>::value ||
                     is_array<T>::value  ||
                     is_union<T>::value  ||
                     is_class<T>::value> {};
	*/
	
	// std::add_rvalue_reference
	template<typename T>
    struct add_rvalue_reference
    {
		typedef T&& type;
	};
	
	template<typename _Tp,
	   bool = !is_reference<_Tp>::value
			&& !is_void<_Tp>::value,
	   bool = is_rvalue_reference<_Tp>::value>
	struct __add_lvalue_reference_helper
	{ typedef _Tp   type; };
	
	template<typename _Tp>
	struct __add_lvalue_reference_helper<_Tp, true, false>
	{ typedef _Tp&   type; };
	
	template<typename _Tp>
	struct __add_lvalue_reference_helper<_Tp, false, true>
	{ typedef typename remove_reference<_Tp>::type&   type; };
	
	// std::add_lvalue_reference
	template <class T>
	struct add_lvalue_reference
		: public __add_lvalue_reference_helper<T>
	{};
	
	// std::add_pointer
	template<class T>
	struct add_pointer
	{
		typedef typename remove_reference<T>::type* type;
	};
	
	// std::declval
	template <typename T>
	typename add_rvalue_reference<T>::type declval();
	
	// std::conditional
	template<bool B, class T, class F>
	struct conditional { typedef T type; };
	
	template<class T, class F>
	struct conditional<false, T, F> { typedef F type; };
	
	// std::decay
	// http://en.cppreference.com/w/cpp/types/decay
	
	template<class T>
	struct decay {
		typedef typename remove_reference<T>::type U;
		typedef typename conditional< 
			is_array<U>::value,
			typename remove_extent<U>::type*,
			typename conditional< 
				is_function<U>::value,
				typename add_pointer<U>::type,
				typename remove_cv<U>::type
			>::type
		>::type type;
	};
}

#include "forward.hpp"
#include "result_of.hpp"

#endif
