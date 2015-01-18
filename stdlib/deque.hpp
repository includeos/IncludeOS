#ifndef STD_DEQUE_HPP
#define STD_DEQUE_HPP

namespace std
{
	template <class T>
	class deque
	{
		typedef T         value_type;
		typedef size_type size_t;
		typedef difference_type ptrdiff_t;
		typedef reference       remove_const<value_type>&;
		typedef const_reference const reference;
		typedef pointer       remove_reference<reference>*; //allocator_traits<Allocator>::pointer;
		typedef const_pointer const pointer; //allocator_traits<Allocator>::const_pointer;
		
		// no iterator definitions atm, because no iterator library yet
		
		
		//! \brief Default constructor. Constructs empty container.
		//deque() : deque( Allocator() ) {}
		explicit deque( /*const Allocator& alloc*/ );
		
		//! \brief Constructs the container with @count copies of elements with value @value.
		deque(size_type count,
				const T& value
				/*const Allocator& alloc = Allocator()*/);
		
		//! \brief Constructs the container with @count default-inserted instances of T. No copies are made.
		explicit deque(size_type count /*, const Allocator& alloc = Allocator()*/ );
		
		//! \brief Destructs the container. The destructors of the elements are called and the used storage is deallocated. 
		//! Note that if the elements are pointers, the pointed-to objects are not destroyed.
		~deque();
		
		//! \brief Replaces the contents of the container.
		void assign(size_type count, const T& value);
		template <class InputIt>
		void assign(InputIt first, InputIt last);
		void assign(initializer_list<T> ilist);
		
		//! \brief Returns a reference to the element at specified location @pos, with bounds checking. 
		//! \warning Should throw std::out_of_range, but we can't do that
		reference       at(size_type index);
		const_reference at(size_type index) const;
		
		//! \brief Returns a reference to the element at specified location @pos. No bounds checking is performed.
		reference       operator[](size_type pos);
		const_reference operator[](size_type pos) const;
		
		//! \brief   Returns a reference to the first element in the container.
		//! \warning Calling front on an empty container is undefined.
		reference front();
		const_reference front() const;
		
		//! \brief   Returns reference to the last element in the container.
		//! \warning Calling back on an empty container is undefined.
		reference back();
		const_reference back() const;
		
		/// iterators going here, at some point ///
		
		//! \brief Checks if the container has no elements, i.e. whether begin() == end().
		bool empty() const noexcept;
		
		//! \brief Returns the number of elements in the container, i.e. std::distance(begin(), end()).
		size_type size() const noexcept;
		
		//! \brief Returns the maximum number of elements the container is able to hold due to system or library implementation limitations.
		size_type max_size() const noexcept
		{
			//return std::numerical_limits<size_type>::max();
			return UINT_MAX;
		}
		
		//! \brief Requests the removal of unused capacity.
		//! It is a non-binding request to reduce capacity to size().
		// NOTE: T must meet the requirements of MoveInsertable. 
		void shrink_to_fit();
		
		//! \brief Removes all elements from the container.
		//! \brief Invalidates any references, pointers, or iterators referring to contained elements.
		void clear();
		
		
	};
	
	template<typename T>
	ostream& operator<<(ostream& s, const deque<T>& v)
	{
		s.put('[');
		char comma[3] = {'\0', ' ', '\0'};
		for (const auto& e : v)
		{
			s << comma << e;
			comma[0] = ',';
		}
		return s << ']';
	}
}

#endif
