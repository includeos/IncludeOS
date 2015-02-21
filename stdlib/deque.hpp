#ifndef STD_DEQUE_HPP
#define STD_DEQUE_HPP

#include <stddef.h>
#include <type_traits.hpp>
#include <initializer_list>
// FIXME: #include <limits>
#include <vector>
#include <list>
#include <string.h>
#include <iostream>

namespace std
{
  class DequeChunkBase
  {
  public:
    typedef size_t size_type;
    
    static const size_type ELEMENTS;
    static const size_type ELEMENTS_SH;
    
    DequeChunkBase() {}
    ~DequeChunkBase() {}
  };
  
  template <class T>
  class DequeChunk : public DequeChunkBase
  {
  public:
    typedef T      value_type;
    typedef T*     pointer;
    
    DequeChunk(int s, int e)
      : start(s), end(e)
    {
      //std::cout << "deque Chunk constructor" << std::endl;
      // allocate elements on stack
      element = new value_type[ELEMENTS];
    }
    // copy-constructor
    DequeChunk(const DequeChunk& c)
      : start(c.start), end(c.end)
    {
      //std::cout << "deque Chunk COPY-constructor" << std::endl;
      element = new value_type[ELEMENTS];
      // straight copy
      memcpy(this->element, c.element, sizeof(value_type) * ELEMENTS);
    }
    ~DequeChunk()
    {
      //std::cout << "deque Chunk destructor" << std::endl;
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
    
    void pop_front()
    {
      // call destructor on front element
      element[this->start].~value_type();
      // "remove" element
      this->start++;
    }
    void pop_back()
    {
      // "remove" element
      this->end--;
      // call destructor
      element[this->end].~value_type();
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
    ~deque() {}
    
    //! \brief Replaces the contents of the container.
    void assign(size_type count, const T& value);
    template <class InputIt>
    void assign(InputIt first, InputIt last);
    void assign(initializer_list<T> ilist);
    
    //! \brief Returns a reference to the element at specified location @pos, with bounds checking. 
    //! \warning Should throw std::out_of_range, but we can't do that...
    reference       at (size_type index);
    const_reference at (size_type index) const;
    
    //! \brief Returns a reference to the element at specified location @pos. No bounds checking is performed.
    reference operator[] (size_type pos)
    {
      size_type first = chunks.front().size();
      // case for result from first chunk
      if (pos < first)
      {
        //std::cout << "Result from first chunk: " << chunks[0].start << " + " << pos << " = " << chunks[0][chunks[0].start + pos] << std::endl;
        return chunks.front()[chunks.front().start + pos];
      }
      
      pos -= first;
      // determine chunk index
      int chunk = 1 + (pos >> chunk_def::ELEMENTS_SH);
      // chunk element index
      size_type idx  = pos & (chunk_def::ELEMENTS-1);
      
      for (auto& ch : chunks)
      {
        if (chunk-- == 0)
          return ch[idx];
      }
      //! @throw error here
      return chunks.front()[idx];
    }
    const_reference operator[] (size_type pos) const
    {
      return const_cast<deque*>(this)->operator[] (pos);
    }
    
    //! \brief   Returns a reference to the first element in the container.
    //! \warning Calling front on an empty container is undefined.
    reference front()
    {
      return chunks.front()[chunks.front().start];
    }
    const_reference front() const
    {
      return chunks.front()[chunks.front().start];
    }
    
    //! \brief   Returns reference to the last element in the container.
    //! \warning Calling back on an empty container is undefined.
    reference back()
    {
      return chunks.back()[chunks.back().end-1];
    }
    const_reference back() const
    {
      return chunks.back()[chunks.back().end-1];
    }
    
    /// iterators going here, at some point ///
    
    //! \brief Checks if the container has no elements, i.e. whether begin() == end().
    bool empty() const noexcept
    {
      return chunks.empty();
    }
    
    //! \brief Returns the number of elements in the container, i.e. std::distance(begin(), end()).
    size_type size() const noexcept
    {
      int len = 0;
      for (auto& chunk : chunks)
        len += chunk.size();
      
      return len;
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
    // NOTE: already fits like a glove, due to no insertion mechanics
    void shrink_to_fit() {}
    
    //! \brief Removes all elements from the container.
    //! \brief Invalidates any references, pointers, or iterators referring to contained elements.
    void clear()
    {
      chunks.clear();
    }
    
    //! \brief Appends the given element value to the end of the container.
    void push_back(const T& value)
    {
      // select free back chunk
      chunk_def& chunk = back_chunk_new();
      // set element at .end to value
      chunk[chunk.end++] = value;
    }
    void push_back(T&& value)
    {
      // select free back chunk
      chunk_def& chunk = back_chunk_new();
      // set element at .end to value
      chunk[chunk.end++] = value;
    }
    
    //! \brief Appends a new element to the end of the container,
    //! which typically uses placement-new to construct the element in-place at the location provided by the container.
    template <class... Args>
    void emplace_back(Args&&... args)
    {
      // select free back chunk
      chunk_def& chunk = back_chunk_new();
      // emplace new T at free spot
      new (&chunk[chunk.end++]) T(*args...);
    }
    
    //! \brief Removes the last element of the container.
    //! \warning Calling pop_back on an empty container is undefined. 
    void pop_back()
    {
      chunks.back().pop_back();
      
      if (chunks.back().empty())
        chunks.pop_back();
    }
    
    //! \brief Prepends the given element value to the beginning of the container.
    //! All iterators, including the past-the-end iterator, are invalidated. No references are invalidated.
    void push_front(const T& value)
    {
      // select free front chunk
      chunk_def& chunk = front_chunk_new();
      // set element at .start to value
      chunk.start--;
      chunk[chunk.start] = value;
    }
    void push_front(T&& value)
    {
      // select free front chunk
      chunk_def& chunk = front_chunk_new();
      // set element at .start to value
      chunk.start--;
      chunk[chunk.start] = value;
    }
    
    //! \brief Inserts a new element to the beginning of the container.
    //! The element is constructed through std::allocator_traits::construct, which typically uses placement-new to construct the element in-place at the location provided by the container. The arguments args... are forwarded to the constructor as std::forward<Args>(args)....
    template <class... Args>
    void emplace_front(Args&&... args)
    {
      // select free front chunk
      chunk_def& chunk = front_chunk_new();
      // emplace new T at free spot
      chunk.start--;
      new (&chunk[chunk.start]) T(*args...);
    }
    
    //! \brief Removes the first element of the container.
    void pop_front()
    {
      chunks.front().pop_front();
      
      if (chunks.front().empty())
        chunks.pop_front();
    }
    
    //! \brief Resizes the container to contain count elements.
    //! If the current size is greater than count, the container is reduced to its first count elements as if by repeatedly calling pop_back().
    void resize(size_type count, const value_type& value)
    {
      int diff = size() - count;
      
      while (diff < 0)
      {
        push_back(value); diff++;
      }
      while (diff > 0)
      {
        pop_back(); diff--;
      }
    }
    void resize(size_type count)
    {
      resize(count, T());
    }
    
    //! \brief Exchanges the contents of the container with those of other. Does not invoke any move, copy, or swap operations on individual elements.
    //! All iterators and references remain valid. The past-the-end iterator is invalidated.
    void swap( deque& other )
    {
      std::swap(this->chunks, other.chunks);
    }
    
  private:
    // find first chunk from the back that contains elements
    chunk_def& back_chunk() const
    {
      return chunks.back();
    }
    // find first chunk from the front that contains elements
    chunk_def& front_chunk() const
    {
      return chunks.front();
    }
    
    // return the first back chunk that has free slots
    chunk_def& back_chunk_new()
    {
      if (chunks.empty())
      {
        chunks.emplace_back(0, 0);
      }
      if (chunks.back().full())
      {
        chunks.emplace_back(0, 0);
      }
      return chunks.back();
    }
    // return the first front chunk that has free slots
    chunk_def& front_chunk_new()
    {
      if (chunks.empty())
      {
        chunks.emplace_front(chunk_def::ELEMENTS, chunk_def::ELEMENTS);
      }
      if (chunks.front().full())
      {
        chunks.emplace_front(chunk_def::ELEMENTS, chunk_def::ELEMENTS);
      }
      return chunks.front();
    }
    
    std::list<chunk_def> chunks;
  };
  
  //! \brief Specializes the std::swap algorithm for std::deque.
  //! Swaps the contents of lhs and rhs. Calls lhs.swap(rhs).
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
