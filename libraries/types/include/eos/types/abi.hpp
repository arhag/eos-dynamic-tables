#pragma once

#include <eos/types/type_id.hpp>
#include <string>
#include <vector>
#include <utility>

namespace eos { namespace types {

   using std::string;
   using std::vector;
   using std::pair;

   struct ABI
   {
      enum type_specification : uint8_t
      {
         struct_type,
         array_type,
         vector_type,
         optional_type,
         variant_type
      };

      struct type_definition
      {
         uint32_t           first;
         int32_t            second;
         type_specification ts;
      };
      
      struct struct_t
      {
         string                        name;
         vector<pair<string, type_id>> fields;
         vector<int16_t>               sort_order;
      };

      struct table_index
      {
         type_id          key_type;
         bool             unique;
         bool             ascending;
         vector<uint16_t> mapping;
     };

      struct table
      {
         type_id::index_t    object_index;
         vector<table_index> indices;
      };


      vector<type_definition> types;
      vector<struct_t>        structs;
      vector<type_id>         variant_cases;
      vector<table>           tables;
   };

} }

/*
EOS_TYPES_REFLECT_STRUCT( type_definition, (first)(second)(ts) );
EOS_TYPES_REFLECT_STRUCT( struct_t, (name)(fields)(sort_order) );
EOS_TYPES_REFLECT_STRUCT( table_index, (key_type)(unique)(ascending)(mapping) );
EOS_TYPES_REFLECT_STRUCT( table, (name)(object_index)(indices) );
EOS_TYPES_REFLECT_STRUCT( ABI, (types)(structs)(variant_cases)(tables) );
*/
