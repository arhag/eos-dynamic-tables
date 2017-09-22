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
         tuple_type,
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
      vector<type_id>         type_sequences;
      vector<table>           tables;
   };

} }


