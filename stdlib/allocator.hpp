#ifndef STD_ALLOCATOR_HPP
#define STD_ALLOCATOR_HPP

#include <stdint.h>
#include "type_traits.hpp"
typedef signed __INTPTR_TYPE__ ptrdiff_t;

namespace std
{
	// std::addressof
	template<typename T>
    inline T* __addressof(T& __r) noexcept
    {
     return reinterpret_cast<T*>
		(&const_cast<char&> 
		(reinterpret_cast<const volatile char&> (__r)));
    }
	
	// std::new_allocator
	template<typename _Tp>
    class new_allocator
    {
    public:
      typedef size_t     size_type;
      typedef ptrdiff_t  difference_type;
      typedef _Tp*       pointer;
      typedef const _Tp* const_pointer;
      typedef _Tp&       reference;
      typedef const _Tp& const_reference;
      typedef _Tp        value_type;

      template<typename _Tp1>
        struct rebind
        { typedef new_allocator<_Tp1> other; };

#if __cplusplus >= 201103L
      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 2103. propagate_on_container_move_assignment
      typedef std::true_type propagate_on_container_move_assignment;
#endif

      new_allocator() noexcept { }

      new_allocator(const new_allocator&) noexcept { }

      template<typename _Tp1>
        new_allocator(const new_allocator<_Tp1>&) noexcept { }

      ~new_allocator() noexcept { }

      pointer
      address(reference __x) const noexcept
      { return std::__addressof(__x); }

      const_pointer
      address(const_reference __x) const noexcept
      { return std::__addressof(__x); }

      // NB: __n is permitted to be 0.  The C++ standard says nothing
      // about what the return value is when __n == 0.
      pointer
      allocate(size_type __n, const void* = 0)
      { 
		if (__n > this->max_size())
			return nullptr; //std::__throw_bad_alloc();

		return static_cast<_Tp*>(::operator new(__n * sizeof(_Tp)));
      }

      // __p is not permitted to be a null pointer.
      void
      deallocate(pointer __p, size_type)
      { ::operator delete(__p); }

      size_type
      max_size() const noexcept
      { return size_t(-1) / sizeof(_Tp); }

#if __cplusplus >= 201103L
      template<typename _Up, typename... _Args>
        void
        construct(_Up* __p, _Args&&... __args)
	{ ::new((void *)__p) _Up(std::forward<_Args>(__args)...); }

      template<typename _Up>
        void 
        destroy(_Up* __p) { __p->~_Up(); }
#else
      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 402. wrong new expression in [some_] allocator::construct
      void 
      construct(pointer __p, const _Tp& __val) 
      { ::new((void *)__p) _Tp(__val); }

      void 
      destroy(pointer __p) { __p->~_Tp(); }
#endif
    };
	
	template<typename _Tp>
    inline bool
    operator==(const new_allocator<_Tp>&, const new_allocator<_Tp>&)
    { return true; }
	
	template<typename _Tp>
    inline bool
    operator!=(const new_allocator<_Tp>&, const new_allocator<_Tp>&)
    { return false; }
	
	template<typename _Tp>
    using __allocator_base = new_allocator<_Tp>;
	
	// std::allocator -->
	template<typename _Tp>
    class allocator;
	
	/// allocator<void> specialization.
	template<>
    class allocator<void>
    {
    public:
      typedef size_t      size_type;
      typedef ptrdiff_t   difference_type;
      typedef void*       pointer;
      typedef const void* const_pointer;
      typedef void        value_type;

      template<typename _Tp1>
        struct rebind
        { typedef allocator<_Tp1> other; };

#if __cplusplus >= 201103L
      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 2103. std::allocator propagate_on_container_move_assignment
      typedef true_type propagate_on_container_move_assignment;
#endif
    };
	
  /**
   * @brief  The @a standard allocator, as per [20.4].
   *
   *  See http://gcc.gnu.org/onlinedocs/libstdc++/manual/bk01pt04ch11.html
   *  for further details.
   *
   *  @tparam  _Tp  Type of allocated object.
   */
	template<typename _Tp>
    class allocator: public __allocator_base<_Tp>
    {
   public:
      typedef size_t     size_type;
      typedef ptrdiff_t  difference_type;
      typedef _Tp*       pointer;
      typedef const _Tp* const_pointer;
      typedef _Tp&       reference;
      typedef const _Tp& const_reference;
      typedef _Tp        value_type;
	  
      template<typename _Tp1>
        struct rebind
        { typedef allocator<_Tp1> other; };

#if __cplusplus >= 201103L
      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 2103. std::allocator propagate_on_container_move_assignment
      typedef true_type propagate_on_container_move_assignment;
#endif
	  
      allocator() throw() { }
	  
      allocator(const allocator& __a) throw()
      : __allocator_base<_Tp>(__a) { }
	  
      template<typename _Tp1>
        allocator(const allocator<_Tp1>&) throw() { }
	  
      ~allocator() throw() { }
	  
      // Inherit everything else.
    };
	
  template<typename _T1, typename _T2>
    inline bool
    operator==(const allocator<_T1>&, const allocator<_T2>&)
    { return true; }
	
  template<typename _Tp>
    inline bool
    operator==(const allocator<_Tp>&, const allocator<_Tp>&)
    { return true; }
	
  template<typename _T1, typename _T2>
    inline bool
    operator!=(const allocator<_T1>&, const allocator<_T2>&)
    { return false; }
	
  template<typename _Tp>
    inline bool
    operator!=(const allocator<_Tp>&, const allocator<_Tp>&)
    { return false; }
	
}

#endif
