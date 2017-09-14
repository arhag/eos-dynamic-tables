#pragma once

#include <type_traits>
#include <cstdint>

namespace eos { namespace types {

   template<typename T, uint8_t start, uint8_t size = sizeof(T)*8, typename S = uint64_t>
   struct bit_view
   {
      static_assert( std::is_integral<T>::value, "T must be an integral type" );
      static_assert( std::is_integral<S>::value && !std::is_signed<S>::value, "S must be an unsigned integral type" );
      static_assert( sizeof(T) <= sizeof(S), "size of T cannot be larger than size of S" );
      static_assert( 0 <= start && start < sizeof(S)*8, "start must be within the internal [0, N) where N = number of bits in S" );
      static_assert( size > 0, "size cannot be 0" );
      static_assert( size != 1 || !std::is_signed<T>::value, "T cannot be signed if size == 1" ); 
      static_assert( size <= sizeof(S)*8, "size is too large" );
      static_assert( start >= sizeof(S)*8 || start + size <= sizeof(S)*8, "size is too large for window to fit into S given the start" );
      static_assert( sizeof(T)*8 >= size, "size is too large to fit into specified type T" );

      using storage_type = S;
      using access_type  = T;

      static const S mask             = ((static_cast<S>(1) << size) - 1) << start;
      static const S sign_check_mask  = (static_cast<S>(1) << (size - 1)) << start;
      static const S add_ones_pattern = ((static_cast<S>(1) << (sizeof(T)*8)) - 1) ^ ((static_cast<S>(1) << size) - 1);
      static const T all_ones         = ((static_cast<T>(1) << size) - 1);

      template<typename U = T>
      static inline 
      typename std::enable_if<!std::is_signed<U>::value, U>::type
      get( S s ) // Expects T to be unsigned
      {
         static_assert( std::is_same<U, T>::value, "Do not override default template argument for get method" );
         
         return static_cast<T>((s & mask) >> start);
      }

      template<typename U = T>
      static inline 
      typename std::enable_if<std::is_signed<U>::value && size == sizeof(U)*8, U>::type
      get( S s ) // Handles special case of signed T (faster implementation than general case)
      {
         static_assert( std::is_same<U, T>::value, "Do not override default template argument for get method" );
    
         return static_cast<T>((s & mask) >> start); 
      }


      template<typename U = T>
      static inline 
      typename std::enable_if<std::is_signed<U>::value && size != sizeof(U)*8, U>::type
      get( S s ) // Handles general case of signed T
      {
         static_assert( std::is_same<U, T>::value, "Do not override default template argument for get method" );

         bool negative = ((sign_check_mask & s) != 0);
         
         S temp = (s & mask) >> start;
         if( negative )
            return static_cast<T>(temp | add_ones_pattern);
         else
            return static_cast<T>(temp);
      }

      static inline void
      set( S& s, T v )
      {
         s = (s & ~mask) | (mask & (static_cast<S>(v) << start)) ;
      }
   };

} }

