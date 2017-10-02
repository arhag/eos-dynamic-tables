#pragma once

// Derived from: https://github.com/llvm-mirror/libcxx/blob/master/include/vector
// Using MIT License. See: https://github.com/llvm-mirror/libcxx/blob/master/LICENSE.TXT

#include <eos/eoslib/type_traits.hpp>
#include <eos/eoslib/new.hpp>
#include <eos/eoslib/limits.hpp>
#include <eos/eoslib/exceptions.hpp>

#include <string.h> // For memcpy

namespace eoslib {

   template <class _Tp> class allocator;

   template <>
   class allocator<void>
   {
   public:
      typedef void*             pointer;
      typedef const void*       const_pointer;
      typedef void              value_type;

      template <class _Up> struct rebind {typedef allocator<_Up> other;};
   };

   template <>
   class allocator<const void>
   {
   public:
      typedef const void*       pointer;
      typedef const void*       const_pointer;
      typedef const void        value_type;

      template <class _Up> struct rebind {typedef allocator<_Up> other;};
   };

   // pointer_traits

   template <class _Tp, class = void> struct __has_element_type : false_type {};

   template <class _Tp> struct __has_element_type<_Tp, typename __void_t<typename _Tp::element_type>::type> : true_type {};

   template <class _Ptr, bool = __has_element_type<_Ptr>::value> struct __pointer_traits_element_type;

   template <class _Ptr>
   struct __pointer_traits_element_type<_Ptr, true>
   {
      typedef typename _Ptr::element_type type;
   };

   template <template <class, class...> class _Sp, class _Tp, class ..._Args>
   struct __pointer_traits_element_type<_Sp<_Tp, _Args...>, true>
   {
      typedef typename _Sp<_Tp, _Args...>::element_type type;
   };

   template <template <class, class...> class _Sp, class _Tp, class ..._Args>
   struct __pointer_traits_element_type<_Sp<_Tp, _Args...>, false>
   {
      typedef _Tp type;
   };

   template <class _Tp, class = void> struct __has_difference_type : false_type {};

   template <class _Tp> struct __has_difference_type<_Tp, typename __void_t<typename _Tp::difference_type>::type> : true_type {};

   template <class _Ptr, bool = __has_difference_type<_Ptr>::value>
   struct __pointer_traits_difference_type
   {
      typedef ptrdiff_t type;
   };

   template <class _Ptr>
   struct __pointer_traits_difference_type<_Ptr, true>
   {
      typedef typename _Ptr::difference_type type;
   };

   template <class _Tp, class _Up>
   struct __has_rebind
   {
   private:
      struct __two {char __lx; char __lxx;};
      template <class _Xp> static __two __test(...);
      template <class _Xp> static char __test(typename _Xp::template rebind<_Up>* = 0);
   public:
      static const bool value = sizeof(__test<_Tp>(0)) == 1;
   };

   template <class _Tp, class _Up, bool = __has_rebind<_Tp, _Up>::value>
   struct __pointer_traits_rebind
   {
      typedef typename _Tp::template rebind<_Up> type;
   };

   template <template <class, class...> class _Sp, class _Tp, class ..._Args, class _Up>
   struct __pointer_traits_rebind<_Sp<_Tp, _Args...>, _Up, true>
   {
      typedef typename _Sp<_Tp, _Args...>::template rebind<_Up> type;
   };

   template <template <class, class...> class _Sp, class _Tp, class ..._Args, class _Up>
   struct __pointer_traits_rebind<_Sp<_Tp, _Args...>, _Up, false>
   {
      typedef _Sp<_Up, _Args...> type;
   };

   template <class _Ptr>
   struct pointer_traits
   {
      typedef _Ptr                                                     pointer;
      typedef typename __pointer_traits_element_type<pointer>::type    element_type;
      typedef typename __pointer_traits_difference_type<pointer>::type difference_type;

      template <class _Up> using rebind = typename __pointer_traits_rebind<pointer, _Up>::type;

   private:
      struct __nat {};
   public:
      static pointer pointer_to(typename conditional<is_void<element_type>::value, __nat, element_type>::type& __r)
      {
         return pointer::pointer_to(__r);
      }
   };

   template <class _Tp>
   struct pointer_traits<_Tp*>
   {
      typedef _Tp*      pointer;
      typedef _Tp       element_type;
      typedef ptrdiff_t difference_type;

      template <class _Up> using rebind = _Up*;

   private:
      struct __nat {};
   public:
      static pointer pointer_to(typename conditional<is_void<element_type>::value, __nat, element_type>::type& __r)
      {
         return addressof(__r);
      }
   };

   template <class _From, class _To>
   struct __rebind_pointer {
      typedef typename pointer_traits<_From>::template rebind<_To> type;
   };

   // allocator_traits

   template <class _Tp, class = void> struct __has_pointer_type : false_type {};

   template <class _Tp> struct __has_pointer_type<_Tp, typename __void_t<typename _Tp::pointer>::type> : true_type {};

   namespace __pointer_type_imp
   {

      template <class _Tp, class _Dp, bool = __has_pointer_type<_Dp>::value>
      struct __pointer_type
      {
         typedef typename _Dp::pointer type;
      };

      template <class _Tp, class _Dp>
      struct __pointer_type<_Tp, _Dp, false>
      {
         typedef _Tp* type;
      };

   }

   template <class _Tp, class _Dp>
   struct __pointer_type
   {
      typedef typename __pointer_type_imp::__pointer_type<_Tp, typename remove_reference<_Dp>::type>::type type;
   };

   template <class _Tp, class = void> struct __has_const_pointer : false_type {};

   template <class _Tp> struct __has_const_pointer<_Tp, typename __void_t<typename _Tp::const_pointer>::type> : true_type {};

   template <class _Tp, class _Ptr, class _Alloc, bool = __has_const_pointer<_Alloc>::value>
   struct __const_pointer
   {
      typedef typename _Alloc::const_pointer type;
   };

   template <class _Tp, class _Ptr, class _Alloc>
   struct __const_pointer<_Tp, _Ptr, _Alloc, false>
   {
      typedef typename pointer_traits<_Ptr>::template rebind<const _Tp> type;
   };

   template <class _Tp, class = void> struct __has_void_pointer : false_type {};

   template <class _Tp> struct __has_void_pointer<_Tp, typename __void_t<typename _Tp::void_pointer>::type> : true_type {};

   template <class _Ptr, class _Alloc, bool = __has_void_pointer<_Alloc>::value>
   struct __void_pointer
   {
      typedef typename _Alloc::void_pointer type;
   };

   template <class _Ptr, class _Alloc>
   struct __void_pointer<_Ptr, _Alloc, false>
   {
      typedef typename pointer_traits<_Ptr>::template rebind<void> type;
   };

   template <class _Tp, class = void> struct __has_const_void_pointer : false_type {};

   template <class _Tp> struct __has_const_void_pointer<_Tp, typename __void_t<typename _Tp::const_void_pointer>::type> : true_type {};

   template <class _Ptr, class _Alloc, bool = __has_const_void_pointer<_Alloc>::value>
   struct __const_void_pointer
   {
      typedef typename _Alloc::const_void_pointer type;
   };

   template <class _Ptr, class _Alloc>
   struct __const_void_pointer<_Ptr, _Alloc, false>
   {
      typedef typename pointer_traits<_Ptr>::template rebind<const void> type;
   };

   template <class _Tp>
   inline
   _Tp*
   __to_raw_pointer(_Tp* __p)
   {
       return __p;
   }

   template <class _Pointer>
   inline
   typename pointer_traits<_Pointer>::element_type*
   __to_raw_pointer(_Pointer __p)
   {
       return __to_raw_pointer(__p.operator->());
   }

   template <class _Tp, class = void> struct __has_size_type : false_type {};

   template <class _Tp> struct __has_size_type<_Tp, typename __void_t<typename _Tp::size_type>::type> : true_type {};

   template <class _Alloc, class _DiffType, bool = __has_size_type<_Alloc>::value>
   struct __size_type
   {
      typedef typename make_unsigned<_DiffType>::type type;
   };

   template <class _Alloc, class _DiffType>
   struct __size_type<_Alloc, _DiffType, true>
   {
      typedef typename _Alloc::size_type type;
   };

   template <class _Tp, class _Up, bool = __has_rebind<_Tp, _Up>::value>
   struct __has_rebind_other
   {
   private:
      struct __two {char __lx; char __lxx;};
      template <class _Xp> static __two __test(...);
      template <class _Xp> static char __test(typename _Xp::template rebind<_Up>::other* = 0);
   public:
      static const bool value = sizeof(__test<_Tp>(0)) == 1;
   };

   template <class _Tp, class _Up>
   struct __has_rebind_other<_Tp, _Up, false>
   {
      static const bool value = false;
   };

   template <class _Tp, class _Up, bool = __has_rebind_other<_Tp, _Up>::value>
   struct __allocator_traits_rebind
   {
      typedef typename _Tp::template rebind<_Up>::other type;
   };

   template <template <class, class...> class _Alloc, class _Tp, class ..._Args, class _Up>
   struct __allocator_traits_rebind<_Alloc<_Tp, _Args...>, _Up, true>
   {
      typedef typename _Alloc<_Tp, _Args...>::template rebind<_Up>::other type;
   };

   template <template <class, class...> class _Alloc, class _Tp, class ..._Args, class _Up>
   struct __allocator_traits_rebind<_Alloc<_Tp, _Args...>, _Up, false>
   {
      typedef _Alloc<_Up, _Args...> type;
   };

   template <class _Alloc, class _SizeType, class _ConstVoidPtr>
   auto __has_allocate_hint_test(_Alloc&& __a, _SizeType&& __sz, _ConstVoidPtr&& __p) -> decltype((void)__a.allocate(__sz, __p), true_type());

   template <class _Alloc, class _SizeType, class _ConstVoidPtr>
   auto __has_allocate_hint_test(const _Alloc& __a, _SizeType&& __sz, _ConstVoidPtr&& __p) -> false_type;

   template <class _Alloc, class _SizeType, class _ConstVoidPtr>
   struct __has_allocate_hint : integral_constant<bool, is_same<decltype(__has_allocate_hint_test(declval<_Alloc>(), 
                                                                                                  declval<_SizeType>(), 
                                                                                                  declval<_ConstVoidPtr>())),
                                                                true_type>::value>
   {};

   template <class _Alloc, class _Tp, class ..._Args>
   decltype(declval<_Alloc>().construct(declval<_Tp*>(), declval<_Args>()...), true_type())
   __has_construct_test(_Alloc&& __a, _Tp* __p, _Args&& ...__args);

   template <class _Alloc, class _Pointer, class ..._Args>
   false_type 
   __has_construct_test(const _Alloc& __a, _Pointer&& __p, _Args&& ...__args);

   template <class _Alloc, class _Pointer, class ..._Args>
   struct __has_construct : integral_constant<bool, is_same<decltype(__has_construct_test(declval<_Alloc>(),
                                                                                          declval<_Pointer>(),
                                                                                          declval<_Args>()...)),
                                                            true_type>::value>
   {};

   template <class _Alloc, class _Pointer>
   auto __has_destroy_test(_Alloc&& __a, _Pointer&& __p) -> decltype(__a.destroy(__p), true_type());

   template <class _Alloc, class _Pointer>
   auto __has_destroy_test(const _Alloc& __a, _Pointer&& __p) -> false_type;

   template <class _Alloc, class _Pointer>
   struct __has_destroy : integral_constant<bool, is_same<decltype(__has_destroy_test(declval<_Alloc>(),
                                                                                      declval<_Pointer>())),
                                                          true_type>::value>
   {};

   template <class _Alloc>
   auto __has_max_size_test(_Alloc&& __a) -> decltype(__a.max_size(), true_type());

   template <class _Alloc>
   auto __has_max_size_test(const volatile _Alloc& __a) -> false_type;

   template <class _Alloc>
   struct __has_max_size : integral_constant<bool, is_same<decltype(__has_max_size_test(declval<_Alloc&>())),
                                                           true_type>::value>
   {};

   template <class _Alloc, class _Ptr, bool = __has_difference_type<_Alloc>::value>
   struct __alloc_traits_difference_type
   {
      typedef typename pointer_traits<_Ptr>::difference_type type;
   };

   template <class _Alloc, class _Ptr>
   struct __alloc_traits_difference_type<_Alloc, _Ptr, true>
   {
      typedef typename _Alloc::difference_type type;
   };

   template <class _Alloc>
   struct allocator_traits
   {
      using allocator_type     = _Alloc;
      using value_type         = typename allocator_type::value_type;
   
      using pointer            = typename __pointer_type<value_type, allocator_type>::type;
      using const_pointer      = typename __const_pointer<value_type, pointer, allocator_type>::type;
      using void_pointer       = typename __void_pointer<pointer, allocator_type>::type;
      using const_void_pointer = typename __const_void_pointer<pointer, allocator_type>::type;

      using difference_type    = typename __alloc_traits_difference_type<allocator_type, pointer>::type;
      using size_type          = typename __size_type<allocator_type, difference_type>::type;

      template <class _Tp> using rebind_alloc  = typename __allocator_traits_rebind<allocator_type, _Tp>::type;
      template <class _Tp> using rebind_traits = allocator_traits<rebind_alloc<_Tp>>;

      static pointer allocate(allocator_type& __a, size_type __n) 
      { 
         return __a.allocate(__n); 
      }

      static pointer allocate(allocator_type& __a, size_type __n, const_void_pointer __hint)
      {
         return __allocate(__a, __n, __hint,  __has_allocate_hint<allocator_type, size_type, const_void_pointer>());
      }

      static void deallocate(allocator_type& __a, pointer __p, size_type __n)
      {
         __a.deallocate(__p, __n);
      }

      template <class _Tp, class... _Args>
      static void construct(allocator_type& __a, _Tp* __p, _Args&&... __args)
      { 
         __construct(__has_construct<allocator_type, _Tp*, _Args...>(), __a, __p, forward<_Args>(__args)...);
      }

      template <class _Tp>
      static void destroy(allocator_type& __a, _Tp* __p)
      {
         __destroy(__has_destroy<allocator_type, _Tp*>(), __a, __p);
      }
      
      static size_type max_size(const allocator_type& __a)
      {
         return __max_size(__has_max_size<const allocator_type>(), __a);
      }

      template <class _Ptr>
      static
      void
      __construct_forward(allocator_type& __a, _Ptr __begin1, _Ptr __end1, _Ptr& __begin2)
      {
         for (; __begin1 != __end1; ++__begin1, ++__begin2)
            construct(__a, __to_raw_pointer(__begin2), move(*__begin1));
      }

      template <class _Tp>
      static
      typename enable_if
      <
         (is_same<allocator_type, allocator<_Tp> >::value
           || !__has_construct<allocator_type, _Tp*, _Tp>::value) &&
         is_trivially_move_constructible<_Tp>::value,
         void
      >::type
      __construct_forward(allocator_type&, _Tp* __begin1, _Tp* __end1, _Tp*& __begin2)
      {
         ptrdiff_t _Np = __end1 - __begin1;
         if (_Np > 0)
         {
            memcpy(__begin2, __begin1, _Np * sizeof(_Tp));
            __begin2 += _Np;
         }
      }

      template <class _Iter, class _Ptr>
      static
      void
      __construct_range_forward(allocator_type& __a, _Iter __begin1, _Iter __end1, _Ptr& __begin2)
      {
         for (; __begin1 != __end1; ++__begin1, (void) ++__begin2)
            construct(__a, __to_raw_pointer(__begin2), *__begin1);
      }

      template <class _Tp>
      static
      typename enable_if
      <
         (is_same<allocator_type, allocator<_Tp> >::value
           || !__has_construct<allocator_type, _Tp*, _Tp>::value) &&
         is_trivially_move_constructible<_Tp>::value,
         void
      >::type
      __construct_range_forward(allocator_type&, _Tp* __begin1, _Tp* __end1, _Tp*& __begin2)
      {
         typedef typename remove_const<_Tp>::type _Vp;
         ptrdiff_t _Np = __end1 - __begin1;
         if (_Np > 0)
         {
            memcpy(const_cast<_Vp*>(__begin2), __begin1, _Np * sizeof(_Tp));
            __begin2 += _Np;
         }
      }

      template <class _Ptr>
      static
      void
      __construct_backward(allocator_type& __a, _Ptr __begin1, _Ptr __end1, _Ptr& __end2)
      {
         while (__end1 != __begin1)
         {
            construct(__a, __to_raw_pointer(__end2-1), move(*--__end1));
            --__end2;
         }
      }

      template <class _Tp>
      static
      typename enable_if
      <
         (is_same<allocator_type, allocator<_Tp> >::value
           || !__has_construct<allocator_type, _Tp*, _Tp>::value) &&
         is_trivially_move_constructible<_Tp>::value,
         void
      >::type
      __construct_backward(allocator_type&, _Tp* __begin1, _Tp* __end1, _Tp*& __end2)
      {
         ptrdiff_t _Np = __end1 - __begin1;
         __end2 -= _Np;
         if (_Np > 0)
            memcpy(__end2, __begin1, _Np * sizeof(_Tp));
      }

   private:

      static pointer __allocate(allocator_type& __a, size_type __n, const_void_pointer __hint, true_type)
      {
         return __a.allocate(__n, __hint);
      }

      static pointer __allocate(allocator_type& __a, size_type __n, const_void_pointer, false_type)
      { 
         return __a.allocate(__n);
      }

      template <class _Tp, class... _Args>
      static void __construct(true_type, allocator_type& __a, _Tp* __p, _Args&&... __args)
      {
         __a.construct(__p, forward<_Args>(__args)...);
      }
      
      template <class _Tp, class... _Args>
      static void __construct(false_type, allocator_type&, _Tp* __p, _Args&&... __args)
      {
         ::new ((void*)__p) _Tp(forward<_Args>(__args)...);
      }

      template <class _Tp>
      static void __destroy(true_type, allocator_type& __a, _Tp* __p)
      {
         __a.destroy(__p);
      }

      template <class _Tp>
      static void __destroy(false_type, allocator_type&, _Tp* __p)
      {
         __p->~_Tp();
      }

      static size_type __max_size(true_type, const allocator_type& __a)
      {
         return __a.max_size();
      }

      static size_type __max_size(false_type, const allocator_type&)
      {
         return numeric_limits<size_type>::max() / sizeof(value_type);
      }

   };

   template <class _Tp>
   class allocator
   {
   public:
      using size_type       = size_t;
      using difference_type = ptrdiff_t;
      using pointer         = _Tp*;
      using const_pointer   = const _Tp*;
      using reference       = _Tp&;
      using const_reference = const _Tp&;
      using value_type      = _Tp;

      template <class _Up> struct rebind {typedef allocator<_Up> other;};

      allocator() {}

      template <class _Up> 
      allocator(const allocator<_Up>&)
      {}

      pointer address(reference __x)const { return addressof(__x); }
      const_pointer address(const_reference __x)const { return addressof(__x); }

      pointer allocate(size_type __n, allocator<void>::const_pointer = 0)
      {
         if (__n > max_size())
            EOS_ERROR(std::length_error, "allocator<T>::allocate(size_t n) 'n' exceeds maximum supported size");

         return static_cast<pointer>(__libcpp_allocate(__n * sizeof(_Tp)));
      }

      void deallocate(pointer __p, size_type)
      {
         __libcpp_deallocate((void*)__p);
      }

      size_type max_size()const
      {
         return size_type(~0) / sizeof(_Tp);
      }

      template <class _Up, class... _Args>
      void
      construct(_Up* __p, _Args&&... __args)
      {
         ::new((void*)__p) _Up(forward<_Args>(__args)...);
      }

      void destroy(pointer __p) 
      {
         __p->~_Tp();
      }

   };

   template <class _Tp>
   class allocator<const _Tp>
   {
   public:
      using size_type       = size_t;
      using difference_type = ptrdiff_t;
      using pointer         = const _Tp*;
      using const_pointer   = const _Tp*;
      using reference       = const _Tp&;
      using const_reference = const _Tp&;
      using value_type      = const _Tp;

      template <class _Up> struct rebind {typedef allocator<_Up> other;};

      allocator() {}

      template <class _Up> 
      allocator(const allocator<_Up>&)
      {}

      const_pointer address(const_reference __x)const { return addressof(__x); }

      pointer allocate(size_type __n, allocator<void>::const_pointer = 0)
      {
         if (__n > max_size())
            EOS_ERROR(std::length_error, "allocator<T>::allocate(size_t n) 'n' exceeds maximum supported size");

         return static_cast<pointer>(__libcpp_allocate(__n * sizeof(_Tp)));
      }

      void deallocate(pointer __p, size_type)
      {
         __libcpp_deallocate((void*) const_cast<_Tp*>(__p));
      }

      size_type max_size()const
      {
         return size_type(~0) / sizeof(_Tp);
      }

      template <class _Up, class... _Args>
      void
      construct(_Up* __p, _Args&&... __args)
      {
         ::new((void*)__p) _Up(forward<_Args>(__args)...);
      }

      void destroy(pointer __p) 
      {
         __p->~_Tp();
      }

   };

   template <class _Tp, class _Up>
   inline
   bool operator==(const allocator<_Tp>&, const allocator<_Up>&) { return true; }

   template <class _Tp, class _Up>
   inline
   bool operator!=(const allocator<_Tp>&, const allocator<_Up>&) { return false; }

}

