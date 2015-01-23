#ifndef STD_DEQUE_HPP
#define STD_DEQUE_HPP

#include <stddef.h>
#include <type_traits.hpp>
#include <initializer_list>
// FIXME: #include <limits>
#include <vector>
#include <string.h>

#include <iostream>

namespace std
{
	template <class T>
	class DequeChunk
	{
	public:
		typedef T      value_type;
		typedef T*     pointer;
		typedef size_t size_type;
		
		static const size_type ELEMENTS    = 8;
		static const size_type ELEMENTS_SH = 3;
		
		DequeChunk()
			: start(0), end(0)
		{
			std::cout << "deque Chunk constructor" << std::endl;
			// allocate elements on stack
			element = new value_type[ELEMENTS];
		}
		// copy-constructor
		DequeChunk(const DequeChunk& c)
			: start(c.start), end(c.end)
		{
			std::cout << "deque Chunk COPY-constructor" << std::endl;
			element = new value_type[ELEMENTS];
			// straight copy
			memcpy(this->element, c.element, sizeof(value_type) * ELEMENTS);
		}
		~DequeChunk()
		{
			std::cout << "deque Chunk destructor" << std::endl;
			// delete & call destructors
			delete[] element;
		}
		
		size_type start;
		size_type end;
		
		size_t size() const noexcept
		{
			return end - start;
		}
		bool full() const noexcept
		{
			return size() == ELEMENTS;
		}
		bool empty() const noexcept
		{
			return size() == 0;
		}
		
		value_type& operator[] (int i)
		{
			return element[i];
		}
		
		void clear()
		{
			this->start = this->end = 0;
		}
		
	private:
		value_type* element;
	};
	
	template <class T>
	class deque
	{
	public:
		typedef T         value_type;
		typedef size_t    size_type;
		typedef ptrdiff_t difference_type;
		typedef value_type&     reference;
		typedef const reference const_reference;
		typedef value_type*     pointer; //allocator_traits<Allocator>::pointer;
		typedef const pointer   const_pointer; //allocator_traits<Allocator>::const_pointer;
		
		typedef DequeChunk<value_type> chunk_def;
		
		// no iterator definitions atm, because no iterator library yet (??)
		
		
		//! \brief Default constructor. Constructs empty container.
		//deque() : deque( Allocator() ) {}
		explicit deque( /*const Allocator& alloc*/ )
		{
			// nada
		}
		
		//! \brief Constructs the container with @count copies of elements with value @value.
		deque(size_type count,
				const T& value
				/*const Allocator& alloc = Allocator()*/)
		{
			while (count--)
				push_back(value);
		}
		
		//! \brief Constructs the container with @count default-inserted instances of T. No copies are made.
		explicit deque(size_type count /*, const Allocator& alloc = Allocator()*/ )
		{
			while (count--)
				push_back(T());
		}
		
		deque(initializer_list<T> init
			  /*const Allocator& alloc = Allocator()*/ )
			// : deque(init.begin(), init.end(), alloc)
		{
			// implement me
		}
		
		//! \brief Destructs the container. The destructors of the elements are called and the used storage is deallocated. 
		//! Note that if the elements are pointers, the pointed-to objects are not destroyed.
		~deque()
		{
			int len = size();
			
			if (len)
			{
				//for (size_type i = 0; i < chunks[0].size(); i++)
				//	delete chunks[0][i];
			}
			
			// nada
		}
		
		//! \brief Replaces the contents of the container.
		void assign(size_type count, const T& value);
		template <class InputIt>
		void assign(InputIt first, InputIt last);
		void assign(initializer_list<T> ilist);
		
		//! \brief Returns a reference to the element at specified location @pos, with bounds checking. 
		//! \warning Should throw std::out_of_range, but we can't do that
		reference       at (size_type index);
		const_reference at (size_type index) const;
		
		//! \brief Returns a reference to the element at specified location @pos. No bounds checking is performed.
		reference operator[] (size_type pos)
		{
			size_type first = chunks[0].size();
			// case for result from first chunk
			if (pos < first) return chunks[0][pos];
			
			pos -= first;
			// determine chunk index
			int chunk = 1 + (pos >> chunk_def::ELEMENTS_SH);
			// chunk element index
			size_type idx  = pos & (chunk_def::ELEMENTS-1);
			return chunks[chunk][idx];
		}
		const_reference operator[] (size_type pos) const
		{
			return const_cast<deque*>(this)->operator[] (pos);
		}
		
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
		bool empty() const noexcept
		{
			return chunks.size() == 0 || (chunks.size() == 1 && chunks[0].size() == 0);
		}
		
		//! \brief Returns the number of elements in the container, i.e. std::distance(begin(), end()).
		size_type size() const noexcept
		{
			if (chunks.size() == 0) return 0;
			if (chunks.size() == 1) return chunks[0].size();
			//     first chunk      + middle chunks                                       + last chunk
			return chunks[0].size() + (chunks.size() - 2) * chunk_def::ELEMENTS + chunks[chunks.size()-1].size();
		}
		
		//! \brief Returns the maximum number of elements the container is able to hold due to system or library implementation limitations.
		size_type max_size() const noexcept
		{
			//return numeric_limits<size_type>::max();
			return UINT_MAX;
		}
		
		//! \brief Requests the removal of unused capacity.
		//! It is a non-binding request to reduce capacity to size().
		// NOTE: T must meet the requirements of MoveInsertable. 
		void shrink_to_fit();
		
		//! \brief Removes all elements from the container.
		//! \brief Invalidates any references, pointers, or iterators referring to contained elements.
		void clear()
		{
			chunks.clear();
		}
		
		//! \brief Appends the given element value to the end of the container.
		void push_back(const T& value)
		{
			make_room(1);
			// select last chunk
			chunk_def& last = chunks[chunks.size()-1];
			
			last[last.end++] = value;
		}
		void push_back(T&& value)
		{
			make_room(1);
			// select last chunk
			chunk_def& last = chunks[chunks.size()-1];
			
			last[last.end++] = value;
		}
		
		//! \brief Appends a new element to the end of the container,
		//! which typically uses placement-new to construct the element in-place at the location provided by the container.
		template <class... Args>
		void emplace_back(Args&&... args)
		{
			make_room(1);
			// select last chunk
			chunk_def& last = chunks[chunks.size()-1];
			
			new (&last[last.end++]) T(*args...);
			//last[last.end++] = T(*args...);
		}
		
		//! \brief Removes the last element of the container.
		//! \warning Calling pop_back on an empty container is undefined. 
		void pop_back();
		
		//! \brief Prepends the given element value to the beginning of the container.
		//! All iterators, including the past-the-end iterator, are invalidated. No references are invalidated.
		void push_front(const T& value);
		void push_front(T&& value);
		
		//! \brief Inserts a new element to the beginning of the container.
		//! The element is constructed through std::allocator_traits::construct, which typically uses placement-new to construct the element in-place at the location provided by the container. The arguments args... are forwarded to the constructor as std::forward<Args>(args)....
		template <class... Args>
		void emplace_front(Args&&... args);
		
		//! \brief Removes the first element of the container.
		void pop_front();
		
		//! \brief Resizes the container to contain count elements.
		//! If the current size is greater than count, the container is reduced to its first count elements as if by repeatedly calling pop_back().
		void resize(size_type count);
		void resize(size_type count, const value_type& value);
		
		//! \brief Exchanges the contents of the container with those of other. Does not invoke any move, copy, or swap operations on individual elements.
		//! All iterators and references remain valid. The past-the-end iterator is invalidated.
		void swap( deque& other );
		
	private:
		// makes room for N elements
		void make_room(int N)
		{
			int total = chunks.size() * chunk_def::ELEMENTS;
			int free  = total - size();
			
			std::cout << "Free: " << free << " N: " << N << std::endl;
			
			if (free < N)
			{
				// chunks needed
				int c = (N - free) / chunk_def::ELEMENTS + 1;
				// add chunks
				while (c--)
					chunks.emplace_back();
			}
		}
		
		std::vector<chunk_def> chunks;
		
	};
	
	//! \brief Specializes the std::swap algorithm for std::deque. Swaps the contents of lhs and rhs. Calls lhs.swap(rhs).
	/*
	template <class T, class Alloc>
	void swap(deque<T, Alloc>& lhs,
			  deque<T, Alloc>& rhs);
	*/
	
	/*
	template <typename T>
	ostream& operator<< (ostream& s, const deque<T>& v)
	{
		s.put('[');
		char comma[3] = {'\0', ' ', '\0'};
		for (const auto& e : v)
		{
			s << comma << e;
			comma[0] = ',';
		}
		return s << ']';
	}*/
}

#endif
