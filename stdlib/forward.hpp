#ifndef STD_FORWARD_HPP
#define STD_FORWARD_HPP

namespace std
{
	namespace detail
	{
		// http://en.cppreference.com/w/cpp/types/remove_reference
		template<class T> struct remove_reference      { typedef T type; };
		template<class T> struct remove_reference<T&>  { typedef T type; };
		template<class T> struct remove_reference<T&&> { typedef T type; };
		
		template<class T>
		using remove_reference_t = typename remove_reference<T>::type;
	}
	
	// std::forward
	template <typename T>
	constexpr T&& forward(detail::remove_reference_t<T>& t) noexcept
	{
		return static_cast<T&&>(t);
	}
	template <typename T>
	constexpr T&& forward(detail::remove_reference_t<T>&& t) noexcept
	{
		return static_cast<T&&>(t);
	}
	
}

#endif
