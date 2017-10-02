#pragma once

// Derived from: https://github.com/llvm-mirror/libcxx/blob/master/include/type_traits
// Using MIT License. See: https://github.com/llvm-mirror/libcxx/blob/master/LICENSE.TXT

#include <eos/eoslib/cstddef.hpp>

namespace eoslib {
   
   struct __two {char __lx[2];};
   
   template <class> struct __void_t { typedef void type; };

   template <class _Tp> _Tp&& __declval(int);
   template <class _Tp> _Tp   __declval(long);

   template <class _Tp> decltype(__declval<_Tp>(0)) declval();

   // addressof

   template <class _Tp>
   inline
   _Tp*
   addressof(_Tp& __x)
   {
      return reinterpret_cast<_Tp *>(const_cast<char *>(&reinterpret_cast<const volatile char &>(__x)));
   }

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

   // __lazy_and

   template <bool _Last, class ..._Preds> struct __lazy_and_impl;

   template <class ..._Preds> struct __lazy_and_impl<false, _Preds...> : false_type {};

   template <> struct __lazy_and_impl<true> : true_type {};

   template <class _Pred> struct __lazy_and_impl<true, _Pred> : integral_constant<bool, _Pred::type::value> {};

   template <class _Hp, class ..._Tp> struct __lazy_and_impl<true, _Hp, _Tp...> : __lazy_and_impl<_Hp::type::value, _Tp...> {};

   template <class _P1, class ..._Pr> struct __lazy_and : __lazy_and_impl<_P1::type::value, _Pr...> {};

   // __lazy_or

   template <bool _List, class ..._Preds> struct __lazy_or_impl;

   template <class ..._Preds> struct __lazy_or_impl<true, _Preds...> : true_type {};

   template <> struct __lazy_or_impl<false> : false_type {};

   template <class _Hp, class ..._Tp> struct __lazy_or_impl<false, _Hp, _Tp...> : __lazy_or_impl<_Hp::type::value, _Tp...> {};

   template <class _P1, class ..._Pr> struct __lazy_or : __lazy_or_impl<_P1::type::value, _Pr...> {}; 

   // __lazy_not

   template <class _Pred> struct __lazy_not : integral_constant<bool, !_Pred::type::value> {};

   // enable_if

   template <bool, class _Tp = void> struct enable_if {};
   template <class _Tp> struct enable_if<true, _Tp> {typedef _Tp type;};

   // conditional
   
   template <bool _Bp, class _If, class _Then> 
   struct conditional 
   {
      typedef _If type;
   };

   template <class _If, class _Then>
   struct conditional<false, _If, _Then>
   {
      typedef _Then type;
   };

   // is_same

   template <class _Tp, class _Up> struct is_same           : public false_type {};
   template <class _Tp>            struct is_same<_Tp, _Tp> : public true_type {};
   
   // is_const
   
   template <class _Tp> struct is_const            : public false_type {};
   template <class _Tp> struct is_const<_Tp const> : public true_type {};

   // is_volatile

   template <class _Tp> struct is_volatile               : public false_type {};
   template <class _Tp> struct is_volatile<_Tp volatile> : public true_type {};

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

   // is_void

   template <class _Tp> struct __libcpp_is_void       : public false_type {};
   template <>          struct __libcpp_is_void<void> : public true_type {};

   template <class _Tp> struct is_void : public __libcpp_is_void<typename remove_cv<_Tp>::type> {};

   // __is_nullptr_t

   template <class _Tp> struct __is_nullptr_t_impl            : public false_type {};
   template <>          struct __is_nullptr_t_impl<nullptr_t> : public true_type {};

   template <class _Tp> struct __is_nullptr_t : public __is_nullptr_t_impl<typename remove_cv<_Tp>::type> {};

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

   // is_floating_point

   template <class _Tp> struct __libcpp_is_floating_point              : public false_type {};
   template <>          struct __libcpp_is_floating_point<float>       : public true_type {};
   template <>          struct __libcpp_is_floating_point<double>      : public true_type {};
   template <>          struct __libcpp_is_floating_point<long double> : public true_type {};

   template <class _Tp> struct is_floating_point : public __libcpp_is_floating_point<typename remove_cv<_Tp>::type> {};

   // is_array

   template <class _Tp>             struct is_array : public false_type {};
   template <class _Tp>             struct is_array<_Tp[]> : public true_type {};
   template <class _Tp, size_t _Np> struct is_array<_Tp[_Np]> : public true_type {};

   // is_pointer

   template <class _Tp> struct __libcpp_is_pointer       : public false_type {};
   template <class _Tp> struct __libcpp_is_pointer<_Tp*> : public true_type {};

   template <class _Tp> struct is_pointer : public __libcpp_is_pointer<typename remove_cv<_Tp>::type> {};

   // is_reference

   template <class _Tp> struct is_lvalue_reference       : public false_type {};
   template <class _Tp> struct is_lvalue_reference<_Tp&> : public true_type {};

   template <class _Tp> struct is_rvalue_reference        : public false_type {};
   template <class _Tp> struct is_rvalue_reference<_Tp&&> : public true_type {};

   template <class _Tp> struct is_reference        : public false_type {};
   template <class _Tp> struct is_reference<_Tp&>  : public true_type {};
   template <class _Tp> struct is_reference<_Tp&&> : public true_type {};

   // is_member_pointer

   template <class _Tp>            struct __libcpp_is_member_pointer             : public false_type {};
   template <class _Tp, class _Up> struct __libcpp_is_member_pointer<_Tp _Up::*> : public true_type {};

   template <class _Tp> struct is_member_pointer : public __libcpp_is_member_pointer<typename remove_cv<_Tp>::type> {};

   // is_union
   
   template <class _Tp> struct __libcpp_union : public false_type {}; // Requires __is_union compiler feature to properly implement
   template <class _Tp> struct is_union : public __libcpp_union<typename remove_cv<_Tp>::type> {};

   // is_class

   namespace __is_class_imp
   {
      template <class _Tp> char  __test(int _Tp::*);
      template <class _Tp> __two __test(...);
   }

   template <class _Tp> struct is_class : public integral_constant<bool, sizeof(__is_class_imp::__test<_Tp>(0)) == 1 && !is_union<_Tp>::value> {};
   
   // is_function

   namespace __libcpp_is_function_imp
   {
      struct __dummy_type {};
      template <class _Tp> char  __test(_Tp*);
      template <class _Tp> char __test(__dummy_type);
      template <class _Tp> __two __test(...);
      template <class _Tp> _Tp&  __source(int);
      template <class _Tp> __dummy_type __source(...);
   }

   template <class _Tp, bool = is_class<_Tp>::value ||
                               is_union<_Tp>::value ||
                               is_void<_Tp>::value  ||
                               is_reference<_Tp>::value ||
                               __is_nullptr_t<_Tp>::value >
   struct __libcpp_is_function : public integral_constant<bool, sizeof(__libcpp_is_function_imp::__test<_Tp>(__libcpp_is_function_imp::__source<_Tp>(0))) == 1> {};

   template <class _Tp> struct __libcpp_is_function<_Tp, true> : public false_type {};

   template <class _Tp> struct is_function : public __libcpp_is_function<_Tp> {};

   // is_enum
 
   template <class _Tp> struct is_enum : public integral_constant<bool, !is_void<_Tp>::value             &&
                                                                        !is_integral<_Tp>::value         &&
                                                                        !is_floating_point<_Tp>::value   &&
                                                                        !is_array<_Tp>::value            &&
                                                                        !is_pointer<_Tp>::value          &&
                                                                        !is_reference<_Tp>::value        &&
                                                                        !is_member_pointer<_Tp>::value   &&
                                                                        !is_union<_Tp>::value            &&
                                                                        !is_class<_Tp>::value            &&
                                                                        !is_function<_Tp>::value         > {};
   // is_arithmetic

   template <class _Tp> struct is_arithmetic : public integral_constant<bool, is_integral<_Tp>::value      ||
                                                                              is_floating_point<_Tp>::value> {};

   // is_fundamental

   template <class _Tp> struct is_fundamental : public integral_constant<bool, is_void<_Tp>::value        ||
                                                                               __is_nullptr_t<_Tp>::value ||
                                                                               is_arithmetic<_Tp>::value> {};

   // is_scalar

   template <class _Tp> struct is_scalar : public integral_constant<bool, is_arithmetic<_Tp>::value     ||
                                                                          is_member_pointer<_Tp>::value ||
                                                                          is_pointer<_Tp>::value        ||
                                                                          __is_nullptr_t<_Tp>::value    ||
                                                                          is_enum<_Tp>::value           > {};

   template <> struct is_scalar<nullptr_t> : public true_type {};

   // __is_referenceable

   struct __is_referenceable_impl {
      template <class _Tp> static _Tp& __test(int);
      template <class _Tp> static __two __test(...);
   };

   template <class _Tp>
   struct __is_referenceable : integral_constant<bool, !is_same<decltype(__is_referenceable_impl::__test<_Tp>(0)), __two>::value> {};

   // add_const

   template <class _Tp, bool = is_reference<_Tp>::value ||
                               is_function<_Tp>::value  ||
                               is_const<_Tp>::value     >
   struct __add_const            
   {
      typedef _Tp type;
   };

   template <class _Tp> 
   struct __add_const<_Tp, false> 
   {
      typedef const _Tp type;
   };

   template <class _Tp> 
   struct add_const
   {
      typedef typename __add_const<_Tp>::type type;
   };

   // add_volatile

   template <class _Tp, bool = is_reference<_Tp>::value ||
                               is_function<_Tp>::value  ||
                               is_volatile<_Tp>::value  >
   struct __add_volatile            
   {
      typedef _Tp type;
   };

   template <class _Tp>
   struct __add_volatile<_Tp, false> 
   {
      typedef volatile _Tp type;
   };

   template <class _Tp> 
   struct add_volatile
   {
      typedef typename __add_volatile<_Tp>::type type;
   };

   // add_cv

   template <class _Tp> 
   struct add_cv
   {
      typedef typename add_const<typename add_volatile<_Tp>::type>::type type;
   };

   // add_lvalue_reference

   template <class _Tp, bool = __is_referenceable<_Tp>::value> struct __add_lvalue_reference_impl            { typedef _Tp  type; };
   template <class _Tp                                       > struct __add_lvalue_reference_impl<_Tp, true> { typedef _Tp& type; };

   template <class _Tp> struct add_lvalue_reference { typedef typename __add_lvalue_reference_impl<_Tp>::type type; };

   // add_rvalue_reference
   template <class _Tp, bool = __is_referenceable<_Tp>::value> struct __add_rvalue_reference_impl            { typedef _Tp   type; };
   template <class _Tp                                       > struct __add_rvalue_reference_impl<_Tp, true> { typedef _Tp&& type; };

   template <class _Tp> struct add_rvalue_reference { typedef typename __add_rvalue_reference_impl<_Tp>::type type; };

   // __uncvref

   template <class _Tp>
   struct __uncvref  {
      typedef typename remove_cv<typename remove_reference<_Tp>::type>::type type;
   };

   template <class _Tp>
   struct __unconstref {
      typedef typename remove_const<typename remove_reference<_Tp>::type>::type type;
   };

   template <class _Tp> using __uncvref_t = typename __uncvref<_Tp>::type;

   // remove_all_extents

   template <class _Tp> 
   struct remove_all_extents
   {
      typedef _Tp type;
   };

   template <class _Tp> 
   struct remove_all_extents<_Tp[]>
   {
      typedef typename remove_all_extents<_Tp>::type type;
   };

   template <class _Tp, size_t _Np> 
   struct remove_all_extents<_Tp[_Np]>
   {
      typedef typename remove_all_extents<_Tp>::type type;
   }; 

   // is_base_of

   namespace __is_base_of_imp
   {
      template <class _Tp>
      struct _Dst
      {
         _Dst(const volatile _Tp &);
      };
      
      template <class _Tp>
      struct _Src
      {
         operator const volatile _Tp &();
         template <class _Up> operator const _Dst<_Up> &();
      };
      
      template <size_t> struct __one { typedef char type; };
      template <class _Bp, class _Dp> typename __one<sizeof(_Dst<_Bp>(declval<_Src<_Dp> >()))>::type __test(int);
      template <class _Bp, class _Dp> __two __test(...);
   }

   template <class _Bp, class _Dp> struct is_base_of : public integral_constant<bool, is_class<_Bp>::value &&
                                                                                      sizeof(__is_base_of_imp::__test<_Bp, _Dp>(0)) == 2>
   {};

   // is_convertible
   
   namespace __is_convertible_imp
   {
      template <class _Tp> void  __test_convert(_Tp);

      template <class _From, class _To, class = void> struct __is_convertible_test : public false_type {};
      template <class _From, class _To> struct __is_convertible_test<_From, _To, decltype(__is_convertible_imp::__test_convert<_To>(declval<_From>()))> : public true_type {};

      template <class _Tp, bool _IsArray =    is_array<_Tp>::value,
                           bool _IsFunction = is_function<_Tp>::value,
                           bool _IsVoid =     is_void<_Tp>::value>
      struct __is_array_function_or_void
      {
         enum {value = 0};
      };

      template <class _Tp> struct __is_array_function_or_void<_Tp, true, false, false> {enum {value = 1};};
      template <class _Tp> struct __is_array_function_or_void<_Tp, false, true, false> {enum {value = 2};};
      template <class _Tp> struct __is_array_function_or_void<_Tp, false, false, true> {enum {value = 3};};
   }

   template <class _Tp, unsigned = __is_convertible_imp::__is_array_function_or_void<typename remove_reference<_Tp>::type>::value>
   struct __is_convertible_check
   {
      static const size_t __v = 0;
   };

   template <class _Tp>
   struct __is_convertible_check<_Tp, 0>
   {
      static const size_t __v = sizeof(_Tp);
   };

   template <class _T1, class _T2, unsigned _T1_is_array_function_or_void = __is_convertible_imp::__is_array_function_or_void<_T1>::value,
                                   unsigned _T2_is_array_function_or_void = __is_convertible_imp::__is_array_function_or_void<_T2>::value>
   struct __is_convertible : public integral_constant<bool, __is_convertible_imp::__is_convertible_test<_T1, _T2>::value>
   {};

   template <class _T1, class _T2> struct __is_convertible<_T1, _T2, 0, 1> : public false_type {};
   template <class _T1, class _T2> struct __is_convertible<_T1, _T2, 1, 1> : public false_type {};
   template <class _T1, class _T2> struct __is_convertible<_T1, _T2, 2, 1> : public false_type {};
   template <class _T1, class _T2> struct __is_convertible<_T1, _T2, 3, 1> : public false_type {};

   template <class _T1, class _T2> struct __is_convertible<_T1, _T2, 0, 2> : public false_type {};
   template <class _T1, class _T2> struct __is_convertible<_T1, _T2, 1, 2> : public false_type {};
   template <class _T1, class _T2> struct __is_convertible<_T1, _T2, 2, 2> : public false_type {};
   template <class _T1, class _T2> struct __is_convertible<_T1, _T2, 3, 2> : public false_type {};

   template <class _T1, class _T2> struct __is_convertible<_T1, _T2, 0, 3> : public false_type {};
   template <class _T1, class _T2> struct __is_convertible<_T1, _T2, 1, 3> : public false_type {};
   template <class _T1, class _T2> struct __is_convertible<_T1, _T2, 2, 3> : public false_type {};
   template <class _T1, class _T2> struct __is_convertible<_T1, _T2, 3, 3> : public true_type {};

   template <class _T1, class _T2> struct is_convertible : public __is_convertible<_T1, _T2>
   {
      static const size_t __complete_check1 = __is_convertible_check<_T1>::__v;
      static const size_t __complete_check2 = __is_convertible_check<_T2>::__v;
   };

   // is_destructible

   //  if it's a reference, return true
   //  if it's a function, return false
   //  if it's   void,     return false
   //  if it's an array of unknown bound, return false
   //  Otherwise, return "std::declval<_Up&>().~_Up()" is well-formed
   //    where _Up is remove_all_extents<_Tp>::type

   template <class> struct __is_destructible_apply { typedef int type; };

   template <typename _Tp>
   struct __is_destructor_wellformed 
   {
      template <typename _Tp1>
      static char  __test(typename __is_destructible_apply<decltype(declval<_Tp1&>().~_Tp1())>::type);

      template <typename _Tp1> static __two __test (...);
    
      static const bool value = sizeof(__test<_Tp>(12)) == sizeof(char);
   };

   template <class _Tp, bool> struct __destructible_imp;

   template <class _Tp>
   struct __destructible_imp<_Tp, false> : public integral_constant<bool, __is_destructor_wellformed<typename remove_all_extents<_Tp>::type>::value> {};

   template <class _Tp> struct __destructible_imp<_Tp, true> : public true_type {};

   template <class _Tp, bool> struct __destructible_false;

   template <class _Tp> struct __destructible_false<_Tp, false> : public __destructible_imp<_Tp, is_reference<_Tp>::value> {};

   template <class _Tp> struct __destructible_false<_Tp, true> : public false_type {};

   template <class _Tp> struct is_destructible : public __destructible_false<_Tp, is_function<_Tp>::value> {};

   template <class _Tp> struct is_destructible<_Tp[]> : public false_type {};

   template <> struct is_destructible<void> : public false_type {};

   // is_constructible
  
   namespace __is_construct
   {
      struct __nat {};
   }

   template <class _Tp, class... _Args> struct __libcpp_is_constructible;

   template <class _To, class _From>
   struct __is_invalid_base_to_derived_cast {
      static_assert(is_reference<_To>::value, "Wrong specialization");
      using _RawFrom = __uncvref_t<_From>;
      using _RawTo = __uncvref_t<_To>;
      static const bool value = __lazy_and<__lazy_not<is_same<_RawFrom, _RawTo>>,
                                           is_base_of<_RawFrom, _RawTo>,
                                           __lazy_not<__libcpp_is_constructible<_RawTo, _From>>
                                          >::value;
   };

   template <class _To, class _From>
   struct __is_invalid_lvalue_to_rvalue_cast : false_type {
      static_assert(is_reference<_To>::value, "Wrong specialization");
   };

   template <class _ToRef, class _FromRef>
   struct __is_invalid_lvalue_to_rvalue_cast<_ToRef&&, _FromRef&> {
      using _RawFrom = __uncvref_t<_FromRef>;
      using _RawTo = __uncvref_t<_ToRef>;
      static const bool value = __lazy_and<__lazy_not<is_function<_RawTo>>,
                                           __lazy_or<is_same<_RawFrom, _RawTo>, is_base_of<_RawTo, _RawFrom>>
                                          >::value;
   };

   struct __is_constructible_helper
   {
      template <class _To>
      static void __eat(_To);

      // This overload is needed to work around a Clang bug that disallows
      // static_cast<T&&>(e) for non-reference-compatible types.
      // Example: static_cast<int&&>(declval<double>());
      // NOTE: The static_cast implementation below is required to support
      //  classes with explicit conversion operators.
      template <class _To, class _From, class = decltype(__eat<_To>(declval<_From>()))>
      static true_type __test_cast(int);

      template <class _To, class _From, class = decltype(static_cast<_To>(declval<_From>()))>
      static integral_constant<bool, !__is_invalid_base_to_derived_cast<_To, _From>::value &&
                                     !__is_invalid_lvalue_to_rvalue_cast<_To, _From>::value
                              > __test_cast(long);

      template <class, class> static false_type __test_cast(...);

      template <class _Tp, class ..._Args, class = decltype(_Tp(declval<_Args>()...))>  static true_type __test_nary(int);
      template <class _Tp, class...> static false_type __test_nary(...);

      template <class _Tp, class _A0, class = decltype(::new _Tp(declval<_A0>()))> static is_destructible<_Tp> __test_unary(int);
      template <class, class> static false_type __test_unary(...);
   };

   template <class _Tp, bool = is_void<_Tp>::value> struct __is_default_constructible : decltype(__is_constructible_helper::__test_nary<_Tp>(0)) {};

   template <class _Tp> struct __is_default_constructible<_Tp, true> : false_type {};

   template <class _Tp> struct __is_default_constructible<_Tp[], false> : false_type {};

   template <class _Tp, size_t _Nx> struct __is_default_constructible<_Tp[_Nx], false> : __is_default_constructible<typename remove_all_extents<_Tp>::type> {};

   template <class _Tp, class... _Args> 
   struct __libcpp_is_constructible
   {
      static_assert(sizeof...(_Args) > 1, "Wrong specialization");
      typedef decltype(__is_constructible_helper::__test_nary<_Tp, _Args...>(0)) type;
   };

   template <class _Tp> struct __libcpp_is_constructible<_Tp> : __is_default_constructible<_Tp> {};

   template <class _Tp, class _A0> struct __libcpp_is_constructible<_Tp, _A0> : public decltype(__is_constructible_helper::__test_unary<_Tp, _A0>(0)) {};

   template <class _Tp, class _A0> struct __libcpp_is_constructible<_Tp&, _A0> : public decltype(__is_constructible_helper::__test_cast<_Tp&, _A0>(0)) {};

   template <class _Tp, class _A0> struct __libcpp_is_constructible<_Tp&&, _A0> : public decltype(__is_constructible_helper::__test_cast<_Tp&&, _A0>(0)) {};

   template <class _Tp, class... _Args> struct is_constructible : public __libcpp_is_constructible<_Tp, _Args...>::type {};

   // is_default_constructible

   template <class _Tp> struct is_default_constructible : public is_constructible<_Tp> {};

   // is_copy_constructible

   template <class _Tp> struct is_copy_constructible : public is_constructible<_Tp, typename add_lvalue_reference<typename add_const<_Tp>::type>::type> {};

   // is_move_constructible

   template <class _Tp> struct is_move_constructible : public is_constructible<_Tp, typename add_rvalue_reference<_Tp>::type> {};

   // is_trivially_constructible

   template <class _Tp, class... _Args>
   struct is_trivially_constructible : false_type {};

   template <class _Tp>
   struct is_trivially_constructible<_Tp> : integral_constant<bool, is_scalar<_Tp>::value> {};

   template <class _Tp>
   struct is_trivially_constructible<_Tp, _Tp&&> : integral_constant<bool, is_scalar<_Tp>::value> {};

   template <class _Tp>
   struct is_trivially_constructible<_Tp, const _Tp&> : integral_constant<bool, is_scalar<_Tp>::value> {};

   template <class _Tp>
   struct is_trivially_constructible<_Tp, _Tp&> : integral_constant<bool, is_scalar<_Tp>::value> {};

   // is_trivially_default_constructible
   
   template <class _Tp> struct is_trivially_default_constructible : public is_trivially_constructible<_Tp> {};

   // is_trivially_copy_constructible

   template <class _Tp> struct is_trivially_copy_constructible : public is_trivially_constructible<_Tp, typename add_lvalue_reference<const _Tp>::type> {};

   // is_trivially_move_constructible

   template <class _Tp> struct is_trivially_move_constructible : public is_trivially_constructible<_Tp, typename add_rvalue_reference<_Tp>::type> {};

   // is_signed

   template <class _Tp, bool = is_integral<_Tp>::value>   struct __libcpp_is_signed_impl : public bool_constant<_Tp(-1) < _Tp(0)> {};
   template <class _Tp>                                   struct __libcpp_is_signed_impl<_Tp, false> : public true_type {};  // floating point

   template <class _Tp, bool = is_arithmetic<_Tp>::value> struct __libcpp_is_signed : public __libcpp_is_signed_impl<_Tp> {};
   template <class _Tp>                                   struct __libcpp_is_signed<_Tp, false> : public false_type {};

   template <class _Tp> struct is_signed : public __libcpp_is_signed<_Tp> {};

   // is_unsigned

   template <class _Tp, bool = is_integral<_Tp>::value>   struct __libcpp_is_unsigned_impl : public bool_constant<_Tp(0) < _Tp(-1)> {};
   template <class _Tp>                                   struct __libcpp_is_unsigned_impl<_Tp, false> : public false_type {};  // floating point

   template <class _Tp, bool = is_arithmetic<_Tp>::value> struct __libcpp_is_unsigned : public __libcpp_is_unsigned_impl<_Tp> {};
   template <class _Tp>                                   struct __libcpp_is_unsigned<_Tp, false> : public false_type {};

   template <class _Tp> struct is_unsigned : public __libcpp_is_unsigned<_Tp> {};

   // make_signed / make_unsigned
   
   template <class _Hp, class _Tp>
   struct __type_list
   {
      typedef _Hp _Head;
      typedef _Tp _Tail;
   };

   struct __nat
   {
      __nat() = delete;
      __nat(const __nat&) = delete;
      __nat& operator=(const __nat&) = delete;
      ~__nat() = delete;
   };

   typedef
      __type_list<signed char,
      __type_list<signed short,
      __type_list<signed int,
      __type_list<signed long,
      __type_list<signed long long,
      __type_list<__int128_t, __nat>
      > > > > > __signed_types;

   typedef
      __type_list<unsigned char,
      __type_list<unsigned short,
      __type_list<unsigned int,
      __type_list<unsigned long,
      __type_list<unsigned long long,
      __type_list<__uint128_t, __nat>
      > > > > > __unsigned_types;

   template <class _TypeList, size_t _Size, bool = _Size <= sizeof(typename _TypeList::_Head)> struct __find_first;

   template <class _Hp, class _Tp, size_t _Size>
   struct __find_first<__type_list<_Hp, _Tp>, _Size, true>
   {
      typedef _Hp type;
   };

   template <class _Hp, class _Tp, size_t _Size>
   struct __find_first<__type_list<_Hp, _Tp>, _Size, false>
   {
      typedef typename __find_first<_Tp, _Size>::type type;
   };

   template <class _Tp, class _Up, bool = is_const<typename remove_reference<_Tp>::type>::value, bool = is_volatile<typename remove_reference<_Tp>::type>::value>
   struct __apply_cv
   {
      typedef _Up type;
   };

   template <class _Tp, class _Up>
   struct __apply_cv<_Tp, _Up, true, false>
   {
      typedef const _Up type;
   };

   template <class _Tp, class _Up>
   struct __apply_cv<_Tp, _Up, false, true>
   {
      typedef volatile _Up type;
   };

   template <class _Tp, class _Up>
   struct __apply_cv<_Tp, _Up, true, true>
   {
      typedef const volatile _Up type;
   };

   template <class _Tp, class _Up>
   struct __apply_cv<_Tp&, _Up, false, false>
   {
      typedef _Up& type;
   };

   template <class _Tp, class _Up>
   struct __apply_cv<_Tp&, _Up, true, false>
   {
      typedef const _Up& type;
   };

   template <class _Tp, class _Up>
   struct __apply_cv<_Tp&, _Up, false, true>
   {
      typedef volatile _Up& type;
   };

   template <class _Tp, class _Up>
   struct __apply_cv<_Tp&, _Up, true, true>
   {
      typedef const volatile _Up& type;
   };

   template <class _Tp, bool = is_integral<_Tp>::value || is_enum<_Tp>::value> struct __make_signed {};

   template <class _Tp>
   struct __make_signed<_Tp, true>
   {
      typedef typename __find_first<__signed_types, sizeof(_Tp)>::type type;
   };

   template <> struct __make_signed<bool,               true> {};
   template <> struct __make_signed<  signed short,     true> {typedef short     type;};
   template <> struct __make_signed<unsigned short,     true> {typedef short     type;};
   template <> struct __make_signed<  signed int,       true> {typedef int       type;};
   template <> struct __make_signed<unsigned int,       true> {typedef int       type;};
   template <> struct __make_signed<  signed long,      true> {typedef long      type;};
   template <> struct __make_signed<unsigned long,      true> {typedef long      type;};
   template <> struct __make_signed<  signed long long, true> {typedef long long type;};
   template <> struct __make_signed<unsigned long long, true> {typedef long long type;};
   template <> struct __make_signed<__int128_t,         true> {typedef __int128_t type;};
   template <> struct __make_signed<__uint128_t,        true> {typedef __int128_t type;};

   template <class _Tp>
   struct make_signed
   {
      typedef typename __apply_cv<_Tp, typename __make_signed<typename remove_cv<_Tp>::type>::type>::type type;
   };

   template <class _Tp, bool = is_integral<_Tp>::value || is_enum<_Tp>::value> struct __make_unsigned {};

   template <class _Tp>
   struct __make_unsigned<_Tp, true>
   {
      typedef typename __find_first<__unsigned_types, sizeof(_Tp)>::type type;
   };

   template <> struct __make_unsigned<bool,               true> {};
   template <> struct __make_unsigned<  signed short,     true> {typedef unsigned short     type;};
   template <> struct __make_unsigned<unsigned short,     true> {typedef unsigned short     type;};
   template <> struct __make_unsigned<  signed int,       true> {typedef unsigned int       type;};
   template <> struct __make_unsigned<unsigned int,       true> {typedef unsigned int       type;};
   template <> struct __make_unsigned<  signed long,      true> {typedef unsigned long      type;};
   template <> struct __make_unsigned<unsigned long,      true> {typedef unsigned long      type;};
   template <> struct __make_unsigned<  signed long long, true> {typedef unsigned long long type;};
   template <> struct __make_unsigned<unsigned long long, true> {typedef unsigned long long type;};
   template <> struct __make_unsigned<__int128_t,         true> {typedef __uint128_t        type;};
   template <> struct __make_unsigned<__uint128_t,        true> {typedef __uint128_t        type;};

   template <class _Tp>
   struct make_unsigned
   {
      typedef typename __apply_cv<_Tp, typename __make_unsigned<typename remove_cv<_Tp>::type>::type>::type type;
   };

   // move and forward

   template <typename T>
   typename remove_reference<T>::type&& move(T&& arg)
   {
     return static_cast<typename remove_reference<T>::type&&>(arg);
   }

   template <class T>
   inline T&& forward(typename remove_reference<T>::type& t) noexcept
   {
      return static_cast<T&&>(t);
   }

   template <class T>
   inline T&& forward(typename remove_reference<T>::type&& t) noexcept
   {
      static_assert(!is_lvalue_reference<T>::value, "Can not forward an rvalue as an lvalue.");
      return static_cast<T&&>(t);
   }

}

