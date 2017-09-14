#pragma once

#include <eos/types/bit_view.hpp>
#include <eos/types/type_id.hpp>

#include <iosfwd>

namespace eos { namespace types {

   class field_metadata
   {
      type_id  _tid;
      uint32_t _extra;

      using sorted_window     = bit_view<bool,     31,  1, uint32_t>;
      using ascending_window  = bit_view<bool,     30,  1, uint32_t>;
      using offset_window     = bit_view<uint32_t,  0, 27, uint32_t>;
 
      field_metadata(uint64_t storage, bool skip_validation); // Private constructor that avoids overhead of validation checks if skip_validation is true

   public:

      enum sort_order : uint8_t
      {
         unsorted = 0,
         ascending,
         descending
      };

      static const uint32_t offset_limit         = (1 << 24);
      static const uint32_t offset_limit_in_bits = (offset_limit << 3);

      field_metadata();
      field_metadata(uint64_t storage);
      field_metadata(type_id tid, uint32_t offset, sort_order so = unsorted);

      inline uint64_t get_storage()const
      {
         return ((static_cast<uint64_t>(_tid.get_storage()) << 32) | _extra);
      }

      inline type_id get_type_id()const
      {
         return _tid;
      }

      inline sort_order get_sort_order()const 
      {
         if( !sorted_window::get(_extra) )
            return unsorted;
         if( ascending_window::get(_extra) )
            return ascending;
         else
            return descending;
      }

      inline void set_sort_order(sort_order so)
      {
         switch( so )
         {
            case unsorted:
               sorted_window::set(_extra, false);
               ascending_window::set(_extra, false);
               break;
            case ascending:
               sorted_window::set(_extra, true);
               ascending_window::set(_extra, true);
               break;
            case descending:
               sorted_window::set(_extra, true);
               ascending_window::set(_extra, false);
               break;
         }
      }

      inline bool is_offset_in_bits()const
      {
         // TODO: return true only for builtin_bool with no containers
         return false;     
      }

      inline uint32_t get_offset()const
      {
         uint32_t offset = offset_window::get(_extra);
         if( is_offset_in_bits() )
            return (offset >> 3);
         else
            return offset;
      }
      
      inline uint32_t get_offset_in_bits()const
      {
         uint32_t offset = offset_window::get(_extra);
         if( is_offset_in_bits() )
            return offset;
         else
            return (offset << 3);
      }

      void set_offset(uint32_t offset);
      void set_offset_in_bits(uint32_t offset);

      friend inline bool operator==(field_metadata lhs, field_metadata rhs) { return (lhs._tid == rhs._tid) && (lhs._extra == rhs._extra); }
      friend inline bool operator!=(field_metadata lhs, field_metadata rhs) { return (lhs._tid != rhs._tid) || (lhs._extra != rhs._extra); }

      friend std::ostream& operator<<(std::ostream& os, const field_metadata&);

   };

} }

