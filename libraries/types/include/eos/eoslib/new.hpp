#pragma once

// Derived from: https://github.com/llvm-mirror/libcxx/blob/master/include/new
// Using MIT License. See: https://github.com/llvm-mirror/libcxx/blob/master/LICENSE.TXT

namespace eoslib {

   inline void* __libcpp_allocate(size_t __size) {
      return ::operator new(__size);
   }

   inline void __libcpp_deallocate(void* __ptr) {
      ::operator delete(__ptr);
   }

}

