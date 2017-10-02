#pragma once

// Derived from: https://github.com/llvm-mirror/libcxx/blob/master/include/utility
// Using MIT License. See: https://github.com/llvm-mirror/libcxx/blob/master/LICENSE.TXT

#include <eos/eoslib/type_traits.hpp>

namespace eoslib {

   template <class _T1, class _T2>
   struct pair
   {
      typedef _T1 first_type;
      typedef _T2 second_type;

      _T1 first;
      _T2 second;

      pair(pair const&) = default;
      pair(pair&&) = default;

      pair() : first(), second() {}

      pair(_T1 const& __t1, _T2 const& __t2) : first(__t1), second(__t2) {}

      template <class _U1, class _U2>
      explicit pair(const pair<_U1, _U2>& __p) : first(__p.first), second(__p.second) {}

      template <class _U1, class _U2>
      explicit pair(pair<_U1, _U2>&& __p) : first(forward<_U1>(__p.first)), second(forward<_U2>(__p.second)) {}

      pair& operator=(pair const& __p) {
         first = __p.first;
         second = __p.second;
         return *this;
      }      

      pair& operator=(pair&& __p) {
         first = forward<first_type>(__p.first);
         second = forward<second_type>(__p.second);
         return *this;
      }      

   };

}

