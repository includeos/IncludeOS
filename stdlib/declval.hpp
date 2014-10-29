#ifndef STD_DECLVAL_HPP
#define STD_DECLVAL_HPP

namespace std
{
	namespace detail
	{
		// std::add_rvalue_reference
		template<typename T>
		struct add_rvalue_reference
		{
			typedef T&& type;
		};
	}
	
	// std::declval
	template <class T>
	typename detail::add_rvalue_reference<T>::type declval();
	
}

#endif
