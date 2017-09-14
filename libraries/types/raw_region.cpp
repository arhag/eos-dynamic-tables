#include <eos/types/raw_region.hpp>
#include <eos/types/field_metadata.hpp>

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
         throw std::invalid_argument("Cannot enlarge raw region to that large of a size.");
      if( new_offset_end <= raw_data.size() )
         return;
      raw_data.resize(new_offset_end);
   }

   void raw_region::clear()
   {
      raw_data.clear();
   }

} }

