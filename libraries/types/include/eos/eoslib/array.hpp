#pragma once

// Derived from: https://github.com/llvm-mirror/libcxx/blob/master/include/array
// Using MIT License. See: https://github.com/llvm-mirror/libcxx/blob/master/LICENSE.TXT

#include <eos/eoslib/type_traits.hpp>
#include <eos/eoslib/cstddef.hpp>
#include <eos/eoslib/iterator.hpp>
#include <eos/eoslib/exceptions.hpp>

namespace eoslib {

   template <class _Tp, size_t _Size>
   struct array
   {
      // types:
      typedef array __self;
      typedef _Tp                                   value_type;
      typedef value_type&                           reference;
      typedef const value_type&                     const_reference;
      typedef value_type*                           iterator;
      typedef const value_type*                     const_iterator;
      typedef value_type*                           pointer;
      typedef const value_type*                     const_pointer;
      typedef size_t                                size_type;
      typedef ptrdiff_t                             difference_type;

      value_type __elems_[_Size > 0 ? _Size : 1];

      // No explicit construct/copy/destroy for aggregate type
      void fill(const value_type& __u) 
      {
         for( size_type i = 0; i < _Size; ++i )
            __elems_[i] = __u;
      }

      /*
      void swap(array& __a)
      { 
         __swap_dispatch((std::integral_constant<bool, _Size == 0>()), __a); 
      }

      void __swap_dispatch(std::true_type, array&) {}

      void __swap_dispatch(std::false_type, array& __a)
      { 
         swap_ranges(__elems_, __elems_ + _Size, __a.__elems_);
      }
      */

      iterator begin() { return iterator(__elems_); }
      const_iterator begin()const { return const_iterator(__elems_); }
      iterator end() { return iterator(__elems_ + _Size); }
      const_iterator end()const { return const_iterator(__elems_ + _Size); }

      const_iterator cbegin()const { return begin(); }
      const_iterator cend()const { return end(); }

      constexpr size_type size()const { return _Size; }
      constexpr size_type max_size()const { return _Size; }
      constexpr bool empty()const { return _Size == 0; }

      reference operator[](size_type __n) { return __elems_[__n]; }
      constexpr const_reference operator[](size_type __n)const { return __elems_[__n]; }

      reference at(size_type __n);
      constexpr const_reference at(size_type __n)const;

      reference front() { return __elems_[0]; }
      constexpr const_reference front()const { return __elems_[0]; }
      reference back()  { return __elems_[_Size > 0 ? _Size-1 : 0]; }
      constexpr const_reference back()const { return __elems_[_Size > 0 ? _Size-1 : 0]; }

      value_type* data() { return __elems_; }
      const value_type* data()const { return __elems_; }
   };

   template <class _Tp, size_t _Size>
   typename array<_Tp, _Size>::reference
   array<_Tp, _Size>::at(size_type __n)
   {
      if (__n >= _Size)
         EOS_ERROR(std::out_of_range, "array::at");

      return __elems_[__n];
   }

   template <class _Tp, size_t _Size>
   constexpr
   typename array<_Tp, _Size>::const_reference
   array<_Tp, _Size>::at(size_type __n) const
   {
      if (__n >= _Size)
         EOS_ERROR(std::out_of_range, "array::at");
      return __elems_[__n];
   }

}

