#pragma once

#include <eos/types/abi_definition.hpp>

#include <set>
#include <map>

namespace eos { namespace types {

   using std::set;
   using std::map;

   class abi_constructor
   {
   public:
      
      abi_constructor() {}

      type_id::index_t declare_struct(const char* name);
      type_id::index_t add_struct(const char* name, const vector<pair<string, type_id>>& fields, const vector<int16_t>& sort_order, const char* base_name = nullptr);
      type_id::index_t add_array(type_id element_type, uint32_t num_elements);
      type_id::index_t add_vector(type_id element_type);
      type_id::index_t add_optional(type_id element_type);
      type_id::index_t add_variant(const vector<type_id>& cases);
      uint32_t         add_table(const char* object_name, const vector<ABI::table_index>& indices); 

      int32_t get_struct_index(const char* name)const; // Returns -1 if no struct found by that name

      inline ABI get_abi()const { return abi; }

   private:

      ABI         abi;

      map<string,                  type_id::index_t>  struct_lookup;
      map<pair<type_id, uint32_t>, type_id::index_t>  array_lookup;
      map<type_id,                 type_id::index_t>  vector_lookup;
      map<type_id,                 type_id::index_t>  optional_lookup;
      map<vector<type_id>,         type_id::index_t>  variant_lookup;

      set<type_id::index_t>             valid_array_start_indices;
      set<type_id::index_t>             valid_vector_start_indices;
      set<type_id::index_t>             valid_sum_type_start_indices; 
 
   };

} }

