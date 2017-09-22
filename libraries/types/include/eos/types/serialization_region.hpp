#pragma once

#include <eos/types/reflect.hpp>
#include <eos/types/raw_region.hpp>
#include <eos/types/types_manager.hpp>
#include <eos/types/deserialize_visitor.hpp>

#include <type_traits>
#include <stdexcept>

namespace eos { namespace types {

   class serialization_region
   {
   public:

      serialization_region(const types_manager& tm, uint32_t initial_capacity = 0)
         : tm(tm)
      {
         if( initial_capacity != 0)
            raw_data.reserve(initial_capacity);
      }

      struct write_visitor
      {
         const types_manager& tm;
         raw_region& r;
         type_id tid;
         uint32_t offset = 0;

         write_visitor( const types_manager& tm, raw_region& r, type_id tid, uint32_t offset)
            : tm(tm), r(r), tid(tid), offset(offset)
         {}
 
         template<typename B>
         typename std::enable_if<std::is_integral<B>::value && !std::is_same<B, bool>::value>::type
         operator()(const B& b)const
         {
            if( tid.get_builtin_type() != eos::types::reflector<B>::builtin_type )
               throw std::runtime_error("Type mismatch");

            r.set<B>(offset, b);
         }

         template<typename B>
         typename std::enable_if<std::is_same<B, bool>::value>::type
         operator()(const B& b)const
         {
            if( tid.get_builtin_type() != eos::types::reflector<B>::builtin_type )
               throw std::runtime_error("Type mismatch");

            r.set<bool>(offset, b); // In this case, offset is in bits.
         }

#undef EOS_TYPES_CUSTOM_BUILTIN_MATCH_START
#undef EOS_TYPES_CUSTOM_BUILTIN_MATCH_END
#undef EOS_TYPES_CUSTOM_BUILTIN_CAST

#define EOS_TYPES_CUSTOM_BUILTIN_MATCH_START( builtin_name )                                                              \
         template<typename B>                                                                                             \
         typename std::enable_if<eos::types::reflector<B>::is_builtin::value && !std::is_integral<B>::value               \
                                 && eos::types::reflector<B>::builtin_type == type_id::builtin_ ## builtin_name >::type   \
         operator()(const B& b)const                                                                                      \
         {                                                                                                                \
            if( tid.get_builtin_type() != eos::types::reflector<B>::builtin_type )                                        \
               throw std::runtime_error("Type mismatch");                                                                 \

#define EOS_TYPES_CUSTOM_BUILTIN_MATCH_END                                                                                \
         }

#define EOS_TYPES_CUSTOM_BUILTIN_CAST( builtin_name, cast_type )                                                          \
   EOS_TYPES_CUSTOM_BUILTIN_MATCH_START( builtin_name )                                                                   \
   this->operator()(static_cast<cast_type>(b));                                                                           \
   EOS_TYPES_CUSTOM_BUILTIN_MATCH_END

EOS_TYPES_CUSTOM_BUILTIN_CAST( int8,   int8_t   )
EOS_TYPES_CUSTOM_BUILTIN_CAST( uint8,  uint8_t  )
EOS_TYPES_CUSTOM_BUILTIN_CAST( int16,  int16_t  )
EOS_TYPES_CUSTOM_BUILTIN_CAST( uint16, uint16_t )
EOS_TYPES_CUSTOM_BUILTIN_CAST( int32,  int32_t  )
EOS_TYPES_CUSTOM_BUILTIN_CAST( uint32, uint32_t )
EOS_TYPES_CUSTOM_BUILTIN_CAST( int64,  int64_t  )
EOS_TYPES_CUSTOM_BUILTIN_CAST( uint64, uint64_t )
EOS_TYPES_CUSTOM_BUILTIN_CAST( bool,   bool     )

EOS_TYPES_CUSTOM_BUILTIN_MATCH_START( string )
   write_vector(b, true);
EOS_TYPES_CUSTOM_BUILTIN_MATCH_END

EOS_TYPES_CUSTOM_BUILTIN_MATCH_START( bytes )
   write_vector(b);
//   throw std::logic_error("Bytes should be represented in C++ by a vector of uint8_t.");
EOS_TYPES_CUSTOM_BUILTIN_MATCH_END

EOS_TYPES_CUSTOM_BUILTIN_MATCH_START( rational )
   r.set<int64_t>(offset, b.numerator); 
   r.set<uint64_t>(offset+8, b.denominator);
EOS_TYPES_CUSTOM_BUILTIN_MATCH_END

EOS_TYPES_CUSTOM_BUILTIN_MATCH_START( any )
   throw std::runtime_error("Not implemented");
EOS_TYPES_CUSTOM_BUILTIN_MATCH_END
   
         template<class Class, class Base>
         typename std::enable_if<eos::types::reflector<Class>::is_struct::value && eos::types::reflector<Base>::is_struct::value>::type
         operator()(const Base& b)const
         {
            auto base_tid = tm.get_member(tid.get_type_index(), 0).get_type_id();
            write_visitor vis(tm, r, base_tid, offset); 
            eos::types::reflector<Base>::visit(b, vis);
         }
 

         write_visitor make_visitor_for_product_type_member( uint32_t member_index )const
         { 
            auto f = tm.get_member(tid.get_type_index(), member_index);
            auto member_tid = f.get_type_id();
            auto member_offset = offset;
            if( member_tid.get_type_class() == type_id::builtin_type && member_tid.get_builtin_type() == type_id::builtin_bool )
            {
               member_offset <<= 3;
               member_offset += f.get_offset_in_bits();
            }
            else
            {
               member_offset += f.get_offset();
            }

            return {tm, r, member_tid, member_offset};
         }

         template<typename Member, class Class, Member (Class::*member)>
         typename std::enable_if<eos::types::reflector<Class>::is_struct::value>::type
         operator()(const Class& c, const char* name, uint32_t member_index)const
         {
            auto vis = make_visitor_for_product_type_member(member_index);
            eos::types::reflector<Member>::visit(c.*member, vis);
         }
         
         template<typename Member, class Class, size_t Index> // Meant for tuples/pairs
         typename std::enable_if<eos::types::reflector<Class>::is_tuple::value>::type
         operator()(const Class& c)const
         {
            auto vis = make_visitor_for_product_type_member(static_cast<uint32_t>(Index));
            eos::types::reflector<Member>::visit(std::get<Index>(c), vis); 
         }
 
         template<class Container>
         typename std::enable_if<eos::types::reflector<Container>::is_array::value>::type
         operator()(const Container& c)const
         {
            type_id element_tid;
            uint32_t num_elements;
            std::tie(element_tid, num_elements) = tm.get_container_element_type(tid);
            if( num_elements < 2 )
               throw std::runtime_error("Type mismatch");
            if( num_elements != c.size() )
               throw std::runtime_error("Mismatch in number of elements of array");

            auto stride = tm.get_size_align(element_tid).get_stride();

            write_visitor vis(tm, r, element_tid, offset); 
            for( uint32_t i = 0; i < num_elements; ++i)
            {
               eos::types::reflector<typename Container::value_type>::visit(c[i], vis);
               vis.offset += stride;
            }
         }

         template<class Container>
         void write_vector(const Container& c, bool write_zero_at_end = false)const
         {
            type_id element_tid;
            uint32_t num_elements;
            std::tie(element_tid, num_elements) = tm.get_container_element_type(tid);
            if( num_elements != 0 )
               throw std::runtime_error("Type mismatch");
            num_elements = c.size();
            if( num_elements == 0 )
               return;

            auto sa = tm.get_size_align(element_tid);
            auto stride = sa.get_stride();
            auto align = sa.get_align();
            
            uint32_t vector_data_offset = type_id::round_up_to_alignment(r.offset_end(), align);
            r.extend( vector_data_offset + ( (write_zero_at_end ? num_elements + 1 : num_elements) * stride) );
            r.set<uint32_t>(offset,   (write_zero_at_end ? num_elements + 1 : num_elements) );
            r.set<uint32_t>(offset+4, vector_data_offset);
 
            write_visitor vis(tm, r, element_tid, vector_data_offset); 
            auto itr = c.begin();
            for( uint32_t i = 0; i < num_elements; ++i, ++itr )
            {
               if( itr == c.end() )
                  throw std::runtime_error("Unexpected end of vector.");

               eos::types::reflector<typename Container::value_type>::visit(*itr, vis);
               vis.offset += stride;
            }
         }


         template<class Container>
         typename std::enable_if<eos::types::reflector<Container>::is_vector::value>::type
         operator()(const Container& c)const
         {
            write_vector(c);
         }

         template<class Container>
         typename std::enable_if<eos::types::reflector<Container>::is_optional::value>::type
         operator()(const Container& c)const
         {
            type_id element_tid;
            uint32_t num_elements;
            std::tie(element_tid, num_elements) = tm.get_container_element_type(tid);
            if( num_elements != 1 )
               throw std::runtime_error("Type mismatch");

            if( !static_cast<bool>(c) )
               return; // Since region is zero initialized, it would be redundant to set optional tag to false.

            auto tag_offset = offset + tm.get_optional_tag_offset(tid);
            r.set<bool>(tag_offset << 3, true);

            write_visitor vis(tm, r, element_tid, offset); 
            eos::types::reflector<typename Container::value_type>::visit(*c, vis);
         }

      };

      template<typename T>
      typename std::enable_if<eos::types::reflector<typename std::remove_cv<T>::type>::is_defined::value, uint32_t>::type
      write_type(const T& type, type_id tid)
      {
         using PlainT = typename std::remove_cv<T>::type;
         auto sa = tm.get_size_align(tid);
         auto starting_offset = type_id::round_up_to_alignment(raw_data.offset_end(), sa.get_align());
         raw_data.extend(starting_offset + sa.get_size());
         write_visitor vis(tm, raw_data, tid, starting_offset);
         eos::types::reflector<PlainT>::visit(type, vis);
         return starting_offset;
      }

      template<typename T>
      typename std::enable_if<eos::types::reflector<typename std::remove_cv<T>::type>::is_defined::value>::type
      read_type(T& type, type_id tid, uint32_t offset = 0)
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

      inline const raw_region& get_raw_region()const { return raw_data; }

      inline raw_region move_raw_region() { return std::move(raw_data); }

      inline void clear() { raw_data.clear(); }

      friend class write_struct_visitor;

   private:
      const types_manager& tm;
      raw_region           raw_data;
   };

} }

