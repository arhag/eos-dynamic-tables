#pragma once

// Derived from: https://github.com/llvm-mirror/libcxx/blob/master/include/limits
// Using MIT License. See: https://github.com/llvm-mirror/libcxx/blob/master/LICENSE.TXT

namespace eoslib {

   template <class _Tp, int __digits, bool _IsSigned>
   struct __libcpp_compute_min
   {
      static constexpr const _Tp value = _Tp(_Tp(1) << __digits);
   };

   template <class _Tp, int __digits>
   struct __libcpp_compute_min<_Tp, __digits, false>
   {
      static constexpr const _Tp value = _Tp(0);
   };

   template <class _Tp>
   class numeric_limits
   {
   public:
      using type = typename enable_if<is_integral<_Tp>::value, _Tp>::type;

      static constexpr const bool is_signed = type(-1) < type(0);
      static constexpr const int  digits = static_cast<int>(sizeof(type) * 8 - is_signed);   
      static constexpr const type __min = __libcpp_compute_min<type, digits, is_signed>::value;
      static constexpr const type __max = is_signed ? type(type(~0) ^ __min) : type(~0);
      static constexpr type min() { return __min; }
      static constexpr type max() { return __max; }      
   };

}

