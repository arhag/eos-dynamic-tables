#pragma once

#include <eos/types/reflect.hpp>
#include <eos/types/raw_region.hpp>
#include <eos/types/types_manager.hpp>
#include <eos/types/deserialize_visitor.hpp>

#include <type_traits>
#include <stdexcept>

namespace eos { namespace types {

   class immutable_region
   {
   public:

      immutable_region(const types_manager& tm, const raw_region& raw_data)
         : tm(tm), raw_data(raw_data)
      {
      }

      immutable_region(const types_manager& tm, raw_region&& raw_data)
         : tm(tm), raw_data(std::move(raw_data))
      {
      }


      //TODO: Complete implementation

      template<typename T>
      typename std::enable_if<eos::types::reflector<typename std::remove_cv<T>::type>::is_struct::value>::type
      read_struct(T& type, type_id tid, uint32_t offset = 0)
      {
         using PlainT = typename std::remove_cv<T>::type;
         auto sa = tm.get_size_align(tid);
         if( offset != type_id::round_up_to_alignment(offset, sa.get_align()) )
            throw std::logic_error("Offset not at appropriate alignment required by the struct.");
         if( raw_data.offset_end() < offset + sa.get_size() )
            throw std::logic_error("Raw data is too small to possibly contain the struct at the specified offset.");
         deserialize_visitor vis(tm, raw_data, tid, offset);
         eos::types::reflector<PlainT>::visit(type, vis);
      }

      inline const vector<byte>& get_raw_data()const { return raw_data.get_raw_data(); }

   private:
      const types_manager& tm;
      raw_region           raw_data;
   };

} }

