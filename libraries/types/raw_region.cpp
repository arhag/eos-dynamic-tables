#include <eos/eoslib/raw_region.hpp>
#include <eos/eoslib/field_metadata.hpp>

#ifdef EOS_TYPES_FULL_CAPABILITY
#include <ostream>
#include <iomanip>
#include <boost/io/ios_state.hpp>
#endif

namespace eos { namespace types {
      
   const byte raw_region::byte_masks[16] = {0x01, 0xFE,   // Mask for each bit position, and its inversion.
                                            0x02, 0xFD,
                                            0x04, 0xFB,
                                            0x08, 0xF7,
                                            0x10, 0xEF,
                                            0x20, 0xDF,
                                            0x40, 0xBF,
                                            0x80, 0x7F };

   void raw_region::reserve(uint32_t new_cap)
   {
      if( new_cap >= field_metadata::offset_limit )
         new_cap = field_metadata::offset_limit;
      raw_data.reserve(new_cap);
   }

   void raw_region::extend(uint32_t new_offset_end)
   {
      if( new_offset_end >= field_metadata::offset_limit )
         EOS_ERROR(std::invalid_argument, "Cannot enlarge raw region to that large of a size.");
      if( new_offset_end <= raw_data.size() )
         return;
      raw_data.resize(new_offset_end);
   }

   void raw_region::clear()
   {
      raw_data.clear();
   }
   
#ifdef EOS_TYPES_FULL_CAPABILITY
   void raw_region::print_raw_data(std::ostream& os, uint32_t offset, uint32_t size)const
   {
      using std::endl;

      boost::io::ios_flags_saver ifs( os );

      if( offset >= offset_end() )
         EOS_ERROR(std::invalid_argument, "Raw region subset is empty.");

      if( size > 4096 )
         EOS_ERROR(std::invalid_argument, "Raw region subset is too large to print.");

      auto col = 0;
      auto row = 0;
      os << std::hex << "    Z = ";
      for( auto i = 0; i < 16; ++i )
      {
         if( i == 8 )
            os << "  ";
         os << i << "  ";
      }
      os << endl << "0xYYZ |";
      os << std::setfill('-') << std::setw(3*16 + 2) << "";
      auto end = get_raw_data().begin() + offset + std::min(size, offset_end() - offset);
      for( auto itr = get_raw_data().begin() + offset; itr < end; ++itr )
      {
         if( col == 0 )
            os << endl << "0x" << std::setfill('0') << std::setw(2) << row << "Z | ";

         os << std::setfill('0') << std::setw(2) << (int) *itr << " ";
         ++col;
         if( col == 16 )
         {
            ++row;
            col = 0;
         }
         else if( col == 8 )
            os << "  ";
      }
      os << endl;
   }

   std::ostream& operator<<(std::ostream& os, const raw_region& r)
   {
      r.print_raw_data(os);
      return os;
   }
#endif

} }

