#pragma once

// Derived from: https://github.com/llvm-mirror/libcxx/blob/master/include/vector
// Using MIT License. See: https://github.com/llvm-mirror/libcxx/blob/master/LICENSE.TXT

#include <eos/eoslib/type_traits.hpp>
#include <eos/eoslib/memory.hpp>
#include <eos/eoslib/limits.hpp>
#include <eos/eoslib/iterator.hpp>
//#include <eos/eoslib/initializer_list.hpp>
#include <eos/eoslib/exceptions.hpp>

namespace eoslib {
 
   template<typename T>
   using initializer_list = std::initializer_list<T>;

   template <class _Tp>
   inline constexpr
   const _Tp&
   __min(const _Tp& __a, const _Tp& __b)
   {
      return (__a < __b) ? __a : __b;
   }

   template <class _Tp>
   inline constexpr
   const _Tp&
   __max(const _Tp& __a, const _Tp& __b)
   {
      return (__a < __b) ? __b : __a;
   }

   template <class _Tp, class _Allocator = allocator<_Tp>>
   class vector
   {
   public:
      using value_type      = _Tp;
      using allocator_type  = _Allocator;
      using __alloc_traits  = allocator_traits<allocator_type>;
      using reference       = value_type&;
      using const_reference = const value_type&;
      using size_type       = typename __alloc_traits::size_type;
      using difference_type = typename __alloc_traits::difference_type;
      using pointer         = typename __alloc_traits::pointer;
      using const_pointer   = typename __alloc_traits::const_pointer;
      using iterator        = pointer;
      using const_iterator  = const_pointer;
   
      static_assert((is_same<typename allocator_type::value_type, value_type>::value),
                    "Allocator::value_type must be same type as value_type");

      vector() 
         : __begin_(nullptr), __end_(nullptr), __end_cap_(nullptr)
      {}

      explicit vector(const allocator_type& __a)
         : __begin_(nullptr), __end_(nullptr), __end_cap_(nullptr), __alloc_(__a)
      {}

      explicit vector(size_type __n);
      vector(size_type __n, const allocator_type& __a);
      vector(size_type __n, const_reference __x);
      vector(size_type __n, const_reference __x, const allocator_type& __a);

      //TODO: Range constructors

      ~vector();

      vector(const vector& __x);
      vector(const vector& __x, const allocator_type& __a);

      vector& operator=(const vector& __x);
      
      vector(initializer_list<value_type> __il);      
      vector(initializer_list<value_type> __il, const allocator_type& __a);

      vector& operator=(initializer_list<value_type> __il)
      {
         assign(__il.begin(), __il.end()); 
         return *this;
      }

      vector(vector&& __x);
      vector(vector&& __x, const allocator_type& __a);

      vector& operator=(vector&& __x);
    
      template <class _InputIterator>
      typename enable_if
      <
         __is_input_iterator  <_InputIterator>::value &&
         !__is_forward_iterator<_InputIterator>::value &&
         is_constructible<value_type, typename iterator_traits<_InputIterator>::reference>::value,
         void
      >::type
      assign(_InputIterator __first, _InputIterator __last);

      template <class _ForwardIterator>
      typename enable_if
      <
         __is_forward_iterator<_ForwardIterator>::value &&
         is_constructible<value_type, typename iterator_traits<_ForwardIterator>::reference>::value,
         void
      >::type
      assign(_ForwardIterator __first, _ForwardIterator __last);

      void assign(size_type __n, const_reference __u);

      void assign(initializer_list<value_type> __il)
      {
         assign(__il.begin(), __il.end());
      }

      inline allocator_type get_allocator()const { return __alloc_; }

      iterator       begin();
      const_iterator begin()const;
      iterator       end();
      const_iterator end()const;
      const_iterator cbegin()const { return begin(); }
      const_iterator cend()const { return end(); }

      //TODO: Reverse iterators
 
      void clear() { __destruct_at_end(__begin_); }

      size_type size()const { return static_cast<size_type>(__end_ - __begin_); }

      size_type capacity()const { return static_cast<size_type>(__end_cap_ - __begin_); } 
      
      bool empty()const { return __begin_ == __end_; }

      size_type max_size()const;
      void reserve(size_type __n);

      reference       operator[](size_type __n);
      const_reference operator[](size_type __n)const;
      reference       at(size_type __n);
      const_reference at(size_type __n)const;
      
      reference front()
      {
         EOS_ASSERT(!empty(), "front() called for empty vector");
         return *__begin_;
      }

      const_reference front()const
      {
         EOS_ASSERT(!empty(), "front() called for empty vector");
         return *__begin_;
      }

      reference back()
      {
         EOS_ASSERT(!empty(), "back() called for empty vector");
         return *(__end_ - 1);
      }

      const_reference back()const
      {
         EOS_ASSERT(!empty(), "back() called for empty vector");
         return *(__end_ - 1);
      }

      value_type* data() { return __to_raw_pointer(this->__begin_); }
      const value_type* data()const { return __to_raw_pointer(this->__begin_); }

      void push_back(const_reference __x);
      void push_back(value_type&& __x);

      template <class... _Args>
      void emplace_back(_Args&&... __args);

      void pop_back();

      //TODO: inserts and erase

      void resize(size_type __sz);
      void resize(size_type __sz, const_reference __x);

      void swap(vector&);

   private:
      pointer __begin_;
      pointer __end_;
      pointer __end_cap_;
      allocator_type __alloc_;
      
      void __throw_length_error()const
      {
         EOS_ERROR(std::length_error, "vector");
      }

      void __throw_out_of_range()const
      {
         EOS_ERROR(std::out_of_range, "vector");
      }

      void __destruct_at_end(pointer __new_last)
      {
         pointer __soon_to_be_end = __end_;
         while (__new_last != __soon_to_be_end)
            __alloc_traits::destroy(__alloc_, __to_raw_pointer(--__soon_to_be_end));
         __end_ = __new_last;
      }
   
      void allocate(size_type __n);
      void deallocate();
      size_type __recommend(size_type __new_size)const;
      void __construct_at_end(size_type __n);
      void __construct_at_end(size_type __n, const_reference __x);

      void __enlarge_buffer(size_type __n);

      void __append(size_type __n);
      void __append(size_type __n, const_reference __x);

   };
      
   //  Allocate space for __n objects
   //  throws length_error if __n > max_size()
   //  throws (probably bad_alloc) if memory run out
   //  Precondition:  __begin_ == __end_ == __end_cap() == 0
   //  Precondition:  __n > 0
   //  Postcondition:  capacity() == __n
   //  Postcondition:  size() == 0
   template <class _Tp, class _Allocator>
   void
   vector<_Tp, _Allocator>::allocate(size_type __n)
   {
       if (__n > max_size())
           __throw_length_error();
       __begin_ = __end_ = __alloc_traits::allocate(__alloc_, __n);
       __end_cap_ = __begin_ + __n;
   }

   template <class _Tp, class _Allocator>
   void
   vector<_Tp, _Allocator>::deallocate()
   {
       if (__begin_ != nullptr)
       {
           clear();
           __alloc_traits::deallocate(__alloc_, __begin_, capacity());
           __begin_ = __end_ = __end_cap_ = nullptr;
       }
   }

   template <class _Tp, class _Allocator>
   typename vector<_Tp, _Allocator>::size_type
   vector<_Tp, _Allocator>::max_size()const
   {
       return __min<size_type>(__alloc_traits::max_size(__alloc_), numeric_limits<difference_type>::max());
   }

   //  Precondition:  __new_size > capacity()
   template <class _Tp, class _Allocator>
   inline
   typename vector<_Tp, _Allocator>::size_type
   vector<_Tp, _Allocator>::__recommend(size_type __new_size)const
   {
       const size_type __ms = max_size();
       if (__new_size > __ms)
           __throw_length_error();
       const size_type __cap = capacity();
       if (__cap >= __ms / 2)
           return __ms;
       return __max<size_type>(2*__cap, __new_size);
   }

   //  Default constructs __n objects starting at __end_
   //  throws if construction throws
   //  Precondition:  __n > 0
   //  Precondition:  size() + __n <= capacity()
   //  Postcondition:  size() == size() + __n
   template <class _Tp, class _Allocator>
   void
   vector<_Tp, _Allocator>::__construct_at_end(size_type __n)
   {
       do
       {
           __alloc_traits::construct(__alloc_, __to_raw_pointer(__end_));
           ++__end_;
           --__n;
       } while (__n > 0);
   }
  
   //  Copy constructs __n objects starting at __end_ from __x
   //  throws if construction throws
   //  Precondition:  __n > 0
   //  Precondition:  size() + __n <= capacity()
   //  Postcondition:  size() == old size() + __n
   //  Postcondition:  [i] == __x for all i in [size() - __n, __n)
   template <class _Tp, class _Allocator>
   inline
   void
   vector<_Tp, _Allocator>::__construct_at_end(size_type __n, const_reference __x)
   {
       do
       {
           __alloc_traits::construct(__alloc_, __to_raw_pointer(__end_), __x);
           ++__end_;
           --__n;
       } while (__n > 0);
   }

   // Constructs a new buffer of specified size and copies existing buffer into it.
   // Precondition: __n > size()
   template <class _Tp, class _Allocator>
   void 
   vector<_Tp, _Allocator>::__enlarge_buffer(size_type __n)
   {
      auto end = __alloc_traits::allocate(__alloc_, __n) + size();
      auto begin = end;
      if (__begin_ != nullptr)
      {
         __alloc_traits::__construct_backward(__alloc_, __begin_, __end_, begin); 
         clear();
         __alloc_traits::deallocate(__alloc_, __begin_, capacity());
      }
      __begin_   = begin;
      __end_     = end;
      __end_cap_ = begin + __n; 
   }

   //  Default constructs __n objects starting at __end_
   //  throws if construction throws
   //  Postcondition:  size() == size() + __n
   //  Exception safety: strong.
   template <class _Tp, class _Allocator>
   void
   vector<_Tp, _Allocator>::__append(size_type __n)
   {
      if (static_cast<size_type>(__end_cap_ - __end_) >= __n)
         __construct_at_end(__n);
      else
      {
         __enlarge_buffer(__recommend(size() + __n));
         __construct_at_end(__n);
      }
   }

   //  Copy constructs __n objects starting at __end_ from __x
   //  throws if construction throws
   //  Postcondition:  size() == size() + __n
   //  Exception safety: strong.
   template <class _Tp, class _Allocator>
   void
   vector<_Tp, _Allocator>::__append(size_type __n, const_reference __x)
   {
      if (static_cast<size_type>(__end_cap_ - __end_) >= __n)
         __construct_at_end(__n, __x);
      else
      {
         __enlarge_buffer(__recommend(size() + __n));
         __construct_at_end(__n, __x);
      }
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>::vector(size_type __n)
      : __begin_(nullptr), __end_(nullptr), __end_cap_(nullptr)
   {
      if (__n > 0)
      {
         allocate(__n);
         __construct_at_end(__n);
      }
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>::vector(size_type __n, const allocator_type& __a)
      : __begin_(nullptr), __end_(nullptr), __end_cap_(nullptr), __alloc_(__a)
   {
      if (__n > 0)
      {
         allocate(__n);
         __construct_at_end(__n);
      }
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>:: vector(size_type __n, const_reference __x)
      : __begin_(nullptr), __end_(nullptr), __end_cap_(nullptr)
   {
      if (__n > 0)
      {
         allocate(__n);
         __construct_at_end(__n, __x);
      }
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>::vector(size_type __n, const_reference __x, const allocator_type& __a)
      : __begin_(nullptr), __end_(nullptr), __end_cap_(nullptr), __alloc_(__a)
   {
      if (__n > 0)
      {
         allocate(__n);
         __construct_at_end(__n, __x);
      }
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>::~vector()
   {
      if (__begin_ != nullptr)
      {
         clear();
         __alloc_traits::deallocate(__alloc_, __begin_, capacity());
      }
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>::vector(const vector& __x)
      : __begin_(nullptr), __end_(nullptr), __end_cap_(nullptr)
   {
      size_type __n = __x.size();
      if (__n > 0)
      {
        allocate(__n);
        __alloc_traits::__construct_range_forward(__alloc_, __x.__begin_, __x.__end_, __end_);
      }
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>::vector(const vector& __x, const allocator_type& __a)
      : __begin_(nullptr), __end_(nullptr), __end_cap_(nullptr), __alloc_(__a)
   {
      size_type __n = __x.size();
      if (__n > 0)
      {
        allocate(__n);
        __alloc_traits::__construct_range_forward(__alloc_, __x.__begin_, __x.__end_, __end_);
      }
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>& 
   vector<_Tp, _Allocator>::operator=(const vector& __x)
   {
      if( this != &__x )
      {
         assign(__x.__begin_, __x.__end_);
      }
      return *this;
   }
   
   template <class _Tp, class _Allocator>
   template <class _InputIterator>
   typename enable_if
   <
      __is_input_iterator  <_InputIterator>::value &&
      !__is_forward_iterator<_InputIterator>::value &&
      is_constructible<_Tp, typename iterator_traits<_InputIterator>::reference>::value,
      void
   >::type
   vector<_Tp, _Allocator>::assign(_InputIterator __first, _InputIterator __last)
   {
      clear();
      for (; __first != __last; ++__first)
         push_back(*__first);
   }
   
   template <class _Tp, class _Allocator>
   template <class _ForwardIterator>
   typename enable_if
   <
      __is_forward_iterator<_ForwardIterator>::value &&
      is_constructible<_Tp, typename iterator_traits<_ForwardIterator>::reference>::value,
      void
   >::type
   vector<_Tp, _Allocator>::assign(_ForwardIterator __first, _ForwardIterator __last)
   {
      // Lazy implementation. Not optimal.
      clear();
      for (; __first != __last; ++__first)
         push_back(*__first);
   }

   template <class _Tp, class _Allocator>
   void vector<_Tp, _Allocator>::assign(size_type __n, const_reference __u)
   {
      if (__n <= capacity())
      {
         size_type __s = size();
         auto c = __min(__n, __s);
         for( auto itr = __begin_; c > 0 ; ++itr, --c )
            *itr = __u;
         if (__n > __s)
            __construct_at_end(__n - __s, __u);
         else
            this->__destruct_at_end(this->__begin_ + __n);
      }
      else
      {
         deallocate();
         allocate(__recommend(static_cast<size_type>(__n)));
         __construct_at_end(__n, __u);
      }
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>::vector(initializer_list<value_type> __il)
      : __begin_(nullptr), __end_(nullptr), __end_cap_(nullptr)
   {
      if (__il.size() > 0)
      {
        allocate(__il.size());
        __alloc_traits::__construct_range_forward(__alloc_, __il.begin(), __il.end(), __end_);
      }
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>::vector(initializer_list<value_type> __il, const allocator_type& __a)
      : __begin_(nullptr), __end_(nullptr), __end_cap_(nullptr), __alloc_(__a)
   {
      if (__il.size() > 0)
      {
        allocate(__il.size());
        __alloc_traits::__construct_range_forward(__alloc_, __il.begin(), __il.end(), __end_);
      }
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>::vector(vector&& __x)
      : __alloc_(move(__x.__alloc_))
   {
      __begin_   = __x.__begin_;
      __end_     = __x.__end_;
      __end_cap_ = __x.__end_cap_;
      __x.__begin_ = __x.__end_ = __x.__end_cap_ = nullptr;
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>::vector(vector&& __x, const allocator_type& __a)
      : __begin_(nullptr), __end_(nullptr), __end_cap_(nullptr), __alloc_(__a)
   {
      if( __a == __x.__alloc_ )
      {
         __begin_   = __x.__begin_;
         __end_     = __x.__end_;
         __end_cap_ = __x.__end_cap_;
         __x.__begin_ = __x.__end_ = __x.__end_cap_ = nullptr;
      }
      else
      {
         EOS_ERROR(std::runtime_error, "Not implemented");
      }
   }

   template <class _Tp, class _Allocator>
   vector<_Tp, _Allocator>& 
   vector<_Tp, _Allocator>::operator=(vector&& __x)
   {
      deallocate();
      __begin_   = __x.__begin_;
      __end_     = __x.__end_;
      __end_cap_ = __x.__end_cap_;
      __x.__begin_ = __x.__end_ = __x.__end_cap_ = nullptr;
      return *this;
   }


   template <class _Tp, class _Allocator>
   typename vector<_Tp, _Allocator>::iterator
   vector<_Tp, _Allocator>::begin()
   {
      return iterator(__begin_);
   }

   template <class _Tp, class _Allocator>
   typename vector<_Tp, _Allocator>::const_iterator
   vector<_Tp, _Allocator>::begin()const
   {
      return const_iterator(__begin_);
   }

   template <class _Tp, class _Allocator>
   typename vector<_Tp, _Allocator>::iterator
   vector<_Tp, _Allocator>::end()
   {
      return iterator(__end_);
   }

   template <class _Tp, class _Allocator>
   typename vector<_Tp, _Allocator>::const_iterator
   vector<_Tp, _Allocator>::end()const
   {
      return const_iterator(__end_);
   }

   template <class _Tp, class _Allocator>
   void
   vector<_Tp, _Allocator>::reserve(size_type __n)
   {
      if (__n > capacity())
      {
         __enlarge_buffer(__n);
      }
   }

   template <class _Tp, class _Allocator>
   typename vector<_Tp, _Allocator>::reference
   vector<_Tp, _Allocator>::operator[](size_type __n)
   {
      return __begin_[__n];
   }

   template <class _Tp, class _Allocator>
   typename vector<_Tp, _Allocator>::const_reference
   vector<_Tp, _Allocator>::operator[](size_type __n)const
   {
      return __begin_[__n];
   }

   template <class _Tp, class _Allocator>
   typename vector<_Tp, _Allocator>::reference
   vector<_Tp, _Allocator>::at(size_type __n)
   {
      if (__n >= size())
         __throw_out_of_range();
      return __begin_[__n];
   }

   template <class _Tp, class _Allocator>
   typename vector<_Tp, _Allocator>::const_reference
   vector<_Tp, _Allocator>::at(size_type __n)const
   {
      if (__n >= size())
         __throw_out_of_range();
      return __begin_[__n];
   }

   template <class _Tp, class _Allocator>
   void
   vector<_Tp, _Allocator>::push_back(const_reference __x)
   {
      if( __end_ == __end_cap_ )
         __enlarge_buffer(size() + 1);

      __alloc_traits::construct(__alloc_, __to_raw_pointer(__end_), __x);
      ++__end_;
   }

   template <class _Tp, class _Allocator>
   void
   vector<_Tp, _Allocator>::push_back(value_type&& __x)
   {
      if( __end_ == __end_cap_ )
         __enlarge_buffer(size() + 1);

      __alloc_traits::construct(__alloc_, __to_raw_pointer(__end_), move(__x));
      ++__end_;
   }

   template <class _Tp, class _Allocator>
   template <class... _Args>
   void
   vector<_Tp, _Allocator>::emplace_back(_Args&&... __args)
   {
      if( __end_ == __end_cap_ )
         __enlarge_buffer(size() + 1);

      __alloc_traits::construct(__alloc_, __to_raw_pointer(__end_), forward<_Args>(__args)...);
      ++__end_;
   }

   template <class _Tp, class _Allocator>
   inline
   void
   vector<_Tp, _Allocator>::pop_back()
   {
      EOS_ASSERT(!empty(), "vector::pop_back called for empty vector");
      __destruct_at_end(__end_ - 1);
   }
   
   template <class _Tp, class _Allocator>
   void
   vector<_Tp, _Allocator>::resize(size_type __sz)
   {
      size_type __cs = size();
      if (__cs < __sz)
         __append(__sz - __cs);
      else if (__cs > __sz)
         __destruct_at_end(__begin_ + __sz);
   }

   template <class _Tp, class _Allocator>
   void
   vector<_Tp, _Allocator>::resize(size_type __sz, const_reference __x)
   {
      size_type __cs = size();
      if (__cs < __sz)
         __append(__sz - __cs, __x);
      else if (__cs > __sz)
         __destruct_at_end(__begin_ + __sz);
   }

   template <class _Tp, class _Allocator>
   void
   vector<_Tp, _Allocator>::swap(vector& __x)
   {
      EOS_ASSERT(__alloc_ == __x.__alloc_, "vector::swap: allocators must compare equal");
      pointer temp;
      temp         = __begin_;
      __begin_     = __x.__begin_;
      __x.__begin_ = temp;
      temp       = __end_;
      __end_     = __x.__end_;
      __x.__end_ = temp;
      temp           = __end_cap_;
      __end_cap_     = __x.__end_cap_;
      __x.__end_cap_ = temp;
   }

}
