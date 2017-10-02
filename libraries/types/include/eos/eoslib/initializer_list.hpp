#pragma once

// Derived from: https://github.com/llvm-mirror/libcxx/blob/master/include/initializer_list
// Using MIT License. See: https://github.com/llvm-mirror/libcxx/blob/master/LICENSE.TXT

// initializer_list is one of few  special cases in the C++ standard library. i
// It must be defined as std::initializer_list because that is what the compiler expects.

//#ifndef EOS_TYPES_REFLET_FULL_CAPABILITY

#include <eos/eoslib/cstddef.hpp>

namespace std { 

   template<class _Ep>
   class initializer_list
   {
      const _Ep* __begin_;
      size_t    __size_;

      constexpr
      initializer_list(const _Ep* __b, size_t __s)
        : __begin_(__b), __size_(__s)
      {}

   public:
      typedef _Ep        value_type;
      typedef const _Ep& reference;
      typedef const _Ep& const_reference;
      typedef size_t    size_type;

      typedef const _Ep* iterator;
      typedef const _Ep* const_iterator;

      constexpr initializer_list() 
         : __begin_(nullptr), __size_(0) 
      {}

      constexpr size_t size()const { return __size_; }
    
      constexpr const _Ep* begin()const { return __begin_; }

      constexpr const _Ep* end()const { return __begin_ + __size_; }
   };

   template<class _Ep>
   inline constexpr
   const _Ep*
   begin(initializer_list<_Ep> __il)
   {
      return __il.begin();
   }

   template<class _Ep>
   inline constexpr
   const _Ep*
   end(initializer_list<_Ep> __il)
   {
      return __il.end();
   }

}

//#endif

