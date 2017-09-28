#include <eos/eoslib/field_metadata.hpp>
#include <eos/eoslib/exceptions.hpp>

#ifdef EOS_TYPES_FULL_CAPABILITY
#include <ostream>
#include <iomanip>
#include <boost/io/ios_state.hpp>
#endif

namespace eos { namespace types {
 
   field_metadata::field_metadata(uint64_t storage, bool skip_validation)
   {
      if( skip_validation )
      {
         _tid = type_id(static_cast<uint32_t>(storage >> 32), true);
         _extra = static_cast<uint32_t>(storage & 0xFFFFFFFF); 
      }
      else
      {
         auto f = field_metadata(storage);
         _tid   = f._tid;
         _extra = f._extra;
      }
   }

   field_metadata::field_metadata(uint64_t storage)
      : _tid(static_cast<uint32_t>(storage >> 32)), _extra(static_cast<uint32_t>(storage & 0xFFFFFFFF))
   {
      if( is_offset_in_bits() )
      {
         if( offset_window::get(_extra) >= offset_limit_in_bits )
            EOS_ERROR(std::invalid_argument, "Offset (specified in bits) is too large.");
      }
      else
      {
         if( offset_window::get(_extra) >= offset_limit )
            EOS_ERROR(std::invalid_argument, "Offset is too large.");
      } 
   }

   field_metadata::field_metadata() 
      : _tid(), _extra(0)
   {} 

  
   field_metadata::field_metadata(type_id tid, uint32_t offset, sort_order so)
      : _tid(tid), _extra(0)
   {
      if( offset >= offset_limit )
         EOS_ERROR(std::domain_error, "Offset is too large"); 

      offset_window::set(_extra, offset);

     if( so != unsorted )
      {
         sorted_window::set(_extra, true);
         ascending_window::set(_extra, (so == ascending));
      }
   }

   void field_metadata::set_offset(uint32_t offset)
   {
      if( offset >= offset_limit )
         EOS_ERROR(std::domain_error, "Offset is too large.");

      if( is_offset_in_bits() )
         offset <<= 3;

      offset_window::set(_extra, offset);
   }

   void field_metadata::set_offset_in_bits(uint32_t offset)
   {
      if( offset >= offset_limit_in_bits )
         EOS_ERROR(std::domain_error, "Offset is too large.");

      if( !is_offset_in_bits() )
         offset >>= 3;

      offset_window::set(_extra, offset);
   }

#ifdef EOS_TYPES_FULL_CAPABILITY
   std::ostream& operator<<(std::ostream& os, const field_metadata& f)
   {
      boost::io::ios_flags_saver ifs( os );

      os << std::dec;
      
      os <<  f.get_type_id();

      // Write sort order information (if sorted)
      auto so = f.get_sort_order();
      if( so != field_metadata::unsorted )
      {
         os << " (sorted in ";
         if( so == field_metadata::ascending )
            os << "ascending";
         else
            os << "descending";
         os << " order)";
      }

      // Write offset  
      os << " located at offset 0x" << std::hex << f.get_offset() << std::dec;

      return os;
   }
#endif

} }

