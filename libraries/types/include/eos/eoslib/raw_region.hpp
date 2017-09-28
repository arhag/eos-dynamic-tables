#pragma once

#include <eos/eoslib/types.h>
#include <eos/eoslib/type_traits.hpp>
#include <eos/eoslib/exceptions.hpp>

#include <vector>
#ifdef EOS_TYPES_FULL_CAPABILITY
#include <iosfwd>
#endif

namespace eos { namespace types {

   using byte = unsigned char;

   using std::vector;

   using eoslib::is_integral;
   using eoslib::is_same;
   using eoslib::enable_if;

   class raw_region
   {
      static const byte byte_masks[16];

   public:

      raw_region() {};

      raw_region(const raw_region& other) = default;
      raw_region& operator=(const raw_region& other) = default;

      raw_region(raw_region&& other)
         : raw_data(std::move(other.raw_data))
      {
         other.raw_data.clear();
      }

      raw_region& operator=(raw_region&& other)
      {
         raw_data = std::move(other.raw_data);
         other.raw_data.clear();
         return *this;
      }

      inline const vector<byte>& get_raw_data()const { return raw_data; }

      inline uint32_t capacity()const   { return raw_data.capacity(); }
      inline uint32_t offset_end()const { return raw_data.size(); }

      void reserve(uint32_t new_cap);
      void extend(uint32_t new_offset_end);
      void clear();

      template<typename T>
      inline
      typename enable_if<is_integral<T>::value && !is_same<T, bool>::value, T>::type
      get( uint32_t offset )const
      {
         if( offset + sizeof(T) > offset_end() )
            EOS_ERROR(std::out_of_range, "Offset puts type outside of current range.");

         return *reinterpret_cast<const T*>(raw_data.data() + offset);
      }

      template<typename T>
      inline
      typename enable_if<is_integral<T>::value && !is_same<T, bool>::value, T&>::type
      get( uint32_t offset )
      {
        if( offset + sizeof(T) > offset_end() )
            EOS_ERROR(std::out_of_range, "Offset puts type outside of current range.");

        return *reinterpret_cast<T*>(raw_data.data() + offset);
      }

      template<typename T>
      inline
      typename enable_if<is_integral<T>::value && !is_same<T, bool>::value>::type
      set( uint32_t offset, T value )
      {
         if( offset + sizeof(T) > offset_end() )
            EOS_ERROR(std::out_of_range, "Offset puts type outside of current range.");

         *reinterpret_cast<T*>(raw_data.data() + offset) = value;
      }

      template<typename T>
      inline
      typename enable_if<is_same<T, bool>::value, bool>::type
      get( uint32_t offset_in_bits )const
      {
         uint32_t offset = (offset_in_bits >> 3);
         if( offset >= offset_end() )
            EOS_ERROR(std::out_of_range, "Offset puts type outside of current range.");

         byte b = *reinterpret_cast<const byte*>(raw_data.data() + offset);
         auto index = ((offset_in_bits & 7) << 1);
         return ((b & byte_masks[index]) != 0);
      }

      template<typename T>
      inline
      typename enable_if<is_same<T, bool>::value>::type
      set( uint32_t offset_in_bits, bool value )
      {
         uint32_t offset = (offset_in_bits >> 3);
         if( offset >= offset_end() )
            EOS_ERROR(std::out_of_range, "Offset puts type outside of current range.");

         byte& b = *reinterpret_cast<byte*>(raw_data.data() + offset);
         auto index = ((offset_in_bits & 7) << 1);
         if( value )
            b |= byte_masks[index];
         else
            b &= byte_masks[index+1];
      }

#ifdef EOS_TYPES_FULL_CAPABILITY
      void print_raw_data(std::ostream& os, uint32_t offset = 0, uint32_t size = 4096 )const;

      friend std::ostream& operator<<(std::ostream& os, const raw_region& r);
#endif

   private:
      vector<byte> raw_data;
   };

} }

