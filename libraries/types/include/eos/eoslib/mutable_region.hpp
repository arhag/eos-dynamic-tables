#pragma once

#include <eos/eoslib/reflect_basic.hpp>
#include <eos/eoslib/raw_region.hpp>
#include <eos/eoslib/full_types_manager.hpp>
#include <eos/eoslib/deserialize_visitor.hpp>
#include <eos/eoslib/exceptions.hpp>
#include <eos/eoslib/type_traits.hpp>

namespace eos { namespace types {

   using eoslib::enable_if;
   using eoslib::remove_cv;

   class mutable_region
   {
   public:

      mutable_region(const full_types_manager& tm, const raw_region& raw_data)
         : tm(tm), raw_data(raw_data)
      {
      }
 
      mutable_region(const full_types_manager& tm, raw_region&& raw_data)
         : tm(tm), raw_data(std::move(raw_data))
      {
      }
 
      //TODO: Complete implementation

      template<typename T>
      typename enable_if<eos::types::reflector<typename remove_cv<T>::type>::is_struct::value>::type
      read_struct(T& type, type_id tid, uint32_t offset = 0)
      {
         using PlainT = typename remove_cv<T>::type;
         auto sa = tm.get_size_align(tid);
         if( offset != type_id::round_up_to_alignment(offset, sa.get_align()) )
            EOS_ERROR(std::logic_error, "Offset not at appropriate alignment required by the struct.");
         if( raw_data.offset_end() < offset + sa.get_size() )
            EOS_ERROR(std::logic_error, "Raw data is too small to possibly contain the struct at the specified offset.");
         deserialize_visitor vis(tm, raw_data, tid, offset);
         eos::types::reflector<PlainT>::visit(type, vis);
      }

      inline const vector<byte>& get_raw_data()const { return raw_data.get_raw_data(); }

   private:
      const full_types_manager& tm;
      raw_region           raw_data;
   };

} }

