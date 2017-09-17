#pragma once

#include <eos/table/dynamic_object.hpp>
#include <eos/types/type_id.hpp>
#include <eos/types/types_manager.hpp>

#include <type_traits>
#include <stdexcept>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

namespace bmi = boost::multi_index;

#define EOS_TABLE_INDEX( r, index, data )                                                                              \
         , bmi::ordered_unique</*bmi::tag<index_num<1+index>>,*/ bmi::identity<dynamic_object>, dynamic_object_compare>

#define EOS_TABLE_MAKE_ALIAS( r, num_indices, data )                                                                   \
   using dynamic_table_##num_indices = bmi::multi_index_container<                                                     \
      dynamic_object,                                                                                                  \
      bmi::indexed_by<                                                                                                 \
         bmi::ordered_unique</*bmi::tag<index_num<0>>,*/ bmi::member<dynamic_object, uint64_t, &dynamic_object::id>>   \
         BOOST_PP_REPEAT(num_indices, EOS_TABLE_INDEX, _)                                                              \
      >                                                                                                                \
   >;

#define EOS_TABLE_INDEX_CTOR( r, index, data )                                                                                        \
         , boost::make_tuple( bmi::identity<dynamic_object>(), dynamic_object_compare( tm.get_table_index(tbl_indx, index) ) ) 

#define EOS_TABLE_CTOR_ARGS_LIST_MAKER( r, num_indices, data )                                                                        \
   template<uint8_t NumIndices>                                                                                                       \
   typename std::enable_if<NumIndices == num_indices, dynamic_table_##num_indices::ctor_args_list>::type                              \
   make_dynamic_table_ctor_args_list(const types_manager& tm, type_id::index_t tbl_indx)                                              \
   {                                                                                                                                  \
      if( tm.get_num_indices_in_table(tbl_indx) != NumIndices )                                                                       \
         throw std::runtime_error("Mismatch between run-time and compile-time evaluation of the number of indices in the table.");    \
      dynamic_table_##num_indices::ctor_args_list args_list =                                                                         \
      boost::make_tuple(                                                                                                              \
         dynamic_table_##num_indices::nth_index<0>::type::ctor_args()                                                                 \
         BOOST_PP_REPEAT(num_indices, EOS_TABLE_INDEX_CTOR, _)                                                                        \
      );                                                                                                                              \
      return args_list;                                                                                                               \
   } 

namespace eos { namespace table {

//   template<uint8_t IndexNum> // IndexNum == 0 is allowed and always represents the id index.
//   struct index_num;

   BOOST_PP_REPEAT(10, EOS_TABLE_MAKE_ALIAS, _) // Limited to 9 user-specified indices (or 10 indices total for Boost.MultiIndex when counting the id index)
                                                // because boost::make_tuple only supports tuples up to a maximum size of 10.

   BOOST_PP_REPEAT(10, EOS_TABLE_CTOR_ARGS_LIST_MAKER, _)

} }

