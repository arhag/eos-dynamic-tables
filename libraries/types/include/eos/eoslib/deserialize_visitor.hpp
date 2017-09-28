#pragma once

#include <eos/eoslib/reflect_basic.hpp>
#include <eos/eoslib/raw_region.hpp>
#include <eos/eoslib/full_types_manager.hpp>
#include <eos/eoslib/exceptions.hpp>
#include <eos/eoslib/type_traits.hpp>

namespace eos { namespace types {

   using eoslib::is_integral;
   using eoslib::is_same; 
   using eoslib::enable_if;

   struct deserialize_visitor
   {
      const full_types_manager& tm;
      const raw_region&  r;
      type_id            tid;
      uint32_t           offset = 0;

      deserialize_visitor( const full_types_manager& tm, const raw_region& r, type_id tid, uint32_t offset)
         : tm(tm), r(r), tid(tid), offset(offset)
      {}

      template<typename B>
      typename enable_if<is_integral<B>::value && !is_same<B, bool>::value>::type
      operator()(B& b)const
      {
         if( tid.get_builtin_type() != eos::types::reflector<B>::builtin_type )
            EOS_ERROR(std::runtime_error, "Type mismatch");

         b = r.get<B>(offset);
      }

      template<typename B>
      typename enable_if<is_same<B, bool>::value>::type
      operator()(B& b)const
      {
         if( tid.get_builtin_type() != eos::types::reflector<B>::builtin_type )
            EOS_ERROR(std::runtime_error, "Type mismatch");

         b = r.get<bool>(offset);
      }

#undef EOS_TYPES_CUSTOM_BUILTIN_MATCH_START
#undef EOS_TYPES_CUSTOM_BUILTIN_MATCH_END
#undef EOS_TYPES_CUSTOM_BUILTIN_PRIMITIVE

#define EOS_TYPES_CUSTOM_BUILTIN_MATCH_START( builtin_name )                                                              \
         template<typename B>                                                                                             \
         typename enable_if<eos::types::reflector<B>::is_builtin::value && !is_integral<B>::value                         \
                                 && eos::types::reflector<B>::builtin_type == type_id::builtin_ ## builtin_name >::type   \
         operator()(B& b)const                                                                                            \
         {                                                                                                                \
            if( tid.get_builtin_type() != eos::types::reflector<B>::builtin_type )                                        \
               EOS_ERROR(std::runtime_error, "Type mismatch");                                                            \

#define EOS_TYPES_CUSTOM_BUILTIN_MATCH_END                                                                                \
         }

#define EOS_TYPES_CUSTOM_BUILTIN_PRIMITIVE( builtin_name, primitive_type )                                                \
   EOS_TYPES_CUSTOM_BUILTIN_MATCH_START( builtin_name )                                                                   \
   b = static_cast<B>(r.get<primitive_type>(offset));                                                                     \
   EOS_TYPES_CUSTOM_BUILTIN_MATCH_END

EOS_TYPES_CUSTOM_BUILTIN_PRIMITIVE( int8,   int8_t   )
EOS_TYPES_CUSTOM_BUILTIN_PRIMITIVE( uint8,  uint8_t  )
EOS_TYPES_CUSTOM_BUILTIN_PRIMITIVE( int16,  int16_t  )
EOS_TYPES_CUSTOM_BUILTIN_PRIMITIVE( uint16, uint16_t )
EOS_TYPES_CUSTOM_BUILTIN_PRIMITIVE( int32,  int32_t  )
EOS_TYPES_CUSTOM_BUILTIN_PRIMITIVE( uint32, uint32_t )
EOS_TYPES_CUSTOM_BUILTIN_PRIMITIVE( int64,  int64_t  )
EOS_TYPES_CUSTOM_BUILTIN_PRIMITIVE( uint64, uint64_t )
EOS_TYPES_CUSTOM_BUILTIN_PRIMITIVE( bool,   bool     )

EOS_TYPES_CUSTOM_BUILTIN_MATCH_START( string )
   read_vector(b, true);
EOS_TYPES_CUSTOM_BUILTIN_MATCH_END

EOS_TYPES_CUSTOM_BUILTIN_MATCH_START( bytes )
   read_vector(b);
//   EOS_ERROR(std::logic_error, "Bytes should be represented in C++ by a vector of uint8_t.");
EOS_TYPES_CUSTOM_BUILTIN_MATCH_END

EOS_TYPES_CUSTOM_BUILTIN_MATCH_START( rational )
   b.numerator   = r.get<int64_t>(offset); 
   b.denominator = r.get<uint64_t>(offset+8);
EOS_TYPES_CUSTOM_BUILTIN_MATCH_END

EOS_TYPES_CUSTOM_BUILTIN_MATCH_START( any )
   EOS_ERROR(std::runtime_error, "Not implemented");
EOS_TYPES_CUSTOM_BUILTIN_MATCH_END
 
      template<class Class, class Base>
      typename enable_if<eos::types::reflector<Class>::is_struct::value && eos::types::reflector<Base>::is_struct::value>::type
      operator()(Base& b)const
      {
         auto base_tid = tm.get_member(tid.get_type_index(), 0).get_type_id();
         deserialize_visitor vis(tm, r, base_tid, offset); 
         eos::types::reflector<Base>::visit(b, vis);
      }

      deserialize_visitor make_visitor_for_product_type_member( uint32_t member_index )const
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
      typename enable_if<eos::types::reflector<Class>::is_struct::value>::type
      operator()(Class& c, const char* name, uint32_t member_index)const
      {
         auto vis = make_visitor_for_product_type_member(member_index);
         eos::types::reflector<Member>::visit(c.*member, vis);
      }
 
      template<typename Member, class Class, size_t Index> // Meant for tuples/pairs
      typename enable_if<eos::types::reflector<Class>::is_tuple::value>::type
      operator()(Class& c)const
      {
         auto vis = make_visitor_for_product_type_member(static_cast<uint32_t>(Index));
         eos::types::reflector<Member>::visit(std::get<Index>(c), vis); 
      }

      template<class Container>
      typename enable_if<eos::types::reflector<Container>::is_array::value>::type
      operator()(Container& c)const
      {
         type_id element_tid;
         uint32_t num_elements;
         std::tie(element_tid, num_elements) = tm.get_container_element_type(tid);
         if( num_elements < 2 )
            EOS_ERROR(std::runtime_error, "Type mismatch");
         if( num_elements != c.size() )
            EOS_ERROR(std::runtime_error, "Mismatch in number of elements of array");

         auto stride = tm.get_size_align(element_tid).get_stride();

         deserialize_visitor vis(tm, r, element_tid, offset); 
         for( uint32_t i = 0; i < num_elements; ++i)
         {
            eos::types::reflector<typename Container::value_type>::visit(c[i], vis);
            vis.offset += stride;
         }
      }

      template<class Container>
      void read_vector(Container& c, bool extra_zero_at_end = false)const
      {
         if( c.size() != 0 )
            EOS_ERROR(std::runtime_error, "Expected vector to construct to be initially empty.");

         type_id element_tid;
         uint32_t num_elements;
         std::tie(element_tid, num_elements) = tm.get_container_element_type(tid);
         if( num_elements != 0 )
            EOS_ERROR(std::runtime_error, "Type mismatch");

         num_elements = r.get<uint32_t>(offset);
         if( num_elements == 0 || (extra_zero_at_end && num_elements == 1) )
            return;

         auto sa = tm.get_size_align(element_tid);
         auto stride = sa.get_stride();
         auto align = sa.get_align();
  
         uint32_t vector_data_offset = r.get<uint32_t>(offset+4);
         if( vector_data_offset != type_id::round_up_to_alignment(vector_data_offset, align) )
            EOS_ERROR(std::logic_error, "Vector data located at offset that does not satisfy alignment requirements for element type.");
         if( (vector_data_offset + (num_elements * stride)) > r.offset_end() )
            EOS_ERROR(std::logic_error, "Raw region is too small to contain this type.");

         deserialize_visitor vis(tm, r, element_tid, vector_data_offset); 
         auto itr = std::inserter(c, c.end());
         if( extra_zero_at_end )
            --num_elements;
         for( uint32_t i = 0; i < num_elements; ++i )
         {
            typename Container::value_type x;
            eos::types::reflector<typename Container::value_type>::visit(x, vis);
            itr = std::move(x);
            vis.offset += stride;
         }
      }

      template<class Container>
      typename enable_if<eos::types::reflector<Container>::is_vector::value>::type
      operator()(Container& c)const
      {
         read_vector(c);
      }

      template<class Container>
      typename enable_if<eos::types::reflector<Container>::is_optional::value>::type
      operator()(Container& c)const
      {
         type_id element_tid;
         uint32_t num_elements;
         std::tie(element_tid, num_elements) = tm.get_container_element_type(tid);
         if( num_elements != 1 )
            EOS_ERROR(std::runtime_error, "Type mismatch");

         if( !static_cast<bool>(c) )
            return; // Since region is zero initialized, it would be redundant to set optional tag to false.

         auto tag_offset = offset + tm.get_optional_tag_offset(tid);
         if( !r.get<bool>(tag_offset << 3) )
            return;

         typename Container::value_type x;
         deserialize_visitor vis(tm, r, element_tid, offset); 
         eos::types::reflector<typename Container::value_type>::visit(x, vis);
         c = std::move(x);
      }

   };

} }

