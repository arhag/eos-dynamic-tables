#pragma once

// Derived from: https://github.com/llvm-mirror/libcxx/blob/master/include/type_traits
// Using MIT License. See: https://github.com/llvm-mirror/libcxx/blob/master/LICENSE.TXT

namespace eoslib {

   // integral_constant

   template <class _Tp, _Tp __v>
   struct integral_constant
   {
      static constexpr const _Tp value = __v;
      typedef _Tp value_type;
      typedef integral_constant type;
   }; 

   template <class _Tp, _Tp __v>
   constexpr const _Tp integral_constant<_Tp, __v>::value;

   // bool_constant and true_type and false_type

   template <bool b>
   using bool_constant = integral_constant<bool, b>;

   typedef bool_constant<true>  true_type;
   typedef bool_constant<false> false_type;

   // remove_const

   template <class _Tp> struct remove_const            {typedef _Tp type;};
   template <class _Tp> struct remove_const<const _Tp> {typedef _Tp type;};

   // remove_volatile

   template <class _Tp> struct remove_volatile               {typedef _Tp type;};
   template <class _Tp> struct remove_volatile<volatile _Tp> {typedef _Tp type;};

   // remove_cv

   template <class _Tp> struct remove_cv {typedef typename remove_volatile<typename remove_const<_Tp>::type>::type type;};

   // remove_reference
   
   template <class _Tp> struct remove_reference        {typedef _Tp type;};
   template <class _Tp> struct remove_reference<_Tp&>  {typedef _Tp type;};
   template <class _Tp> struct remove_reference<_Tp&&> {typedef _Tp type;};

   // enable_if

   template <bool, class _Tp = void> struct enable_if {};
   template <class _Tp> struct enable_if<true, _Tp> {typedef _Tp type;};

   // is_integral
   
   template <class _Tp> struct __libcpp_is_integral                     : public false_type {};
   template <>          struct __libcpp_is_integral<bool>               : public true_type {};
   template <>          struct __libcpp_is_integral<char>               : public true_type {};
   template <>          struct __libcpp_is_integral<signed char>        : public true_type {};
   template <>          struct __libcpp_is_integral<unsigned char>      : public true_type {};
   template <>          struct __libcpp_is_integral<short>              : public true_type {};
   template <>          struct __libcpp_is_integral<unsigned short>     : public true_type {};
   template <>          struct __libcpp_is_integral<int>                : public true_type {};
   template <>          struct __libcpp_is_integral<unsigned int>       : public true_type {};
   template <>          struct __libcpp_is_integral<long>               : public true_type {};
   template <>          struct __libcpp_is_integral<unsigned long>      : public true_type {};
   template <>          struct __libcpp_is_integral<long long>          : public true_type {};
   template <>          struct __libcpp_is_integral<unsigned long long> : public true_type {};
   template <>          struct __libcpp_is_integral<__int128_t>         : public true_type {};
   template <>          struct __libcpp_is_integral<__uint128_t>        : public true_type {};

   template <class _Tp> struct is_integral
       : public __libcpp_is_integral<typename remove_cv<_Tp>::type> {};

   // is_signed (easier implementation since float types are not supported) 

   template <class _Tp> struct is_signed : public bool_constant<_Tp(-1) < _Tp(0)> { typedef typename enable_if<is_integral<_Tp>::value>::type enable_if_t; };

   // is_unsigned (easier implementation since float types are not supported)

   template <class _Tp> struct is_unsigned : public bool_constant<_Tp(0) < _Tp(-1)> { typedef typename enable_if<is_integral<_Tp>::value>::type enable_if_t; };

   // is_same

   template <class _Tp, class _Up> struct is_same           : public false_type {};
   template <class _Tp>            struct is_same<_Tp, _Tp> : public true_type {};

}

