#pragma once

#include <eos/types/type_id.hpp>
#include <eos/types/field_metadata.hpp>
#include <eos/types/abi.hpp>
#include <eos/types/reflect.hpp>
#include <eos/types/types_manager.hpp>
#include <eos/types/full_types_manager.hpp>

#include <utility>
#include <string>
#include <vector>
#include <set>
#include <map>

namespace eos { namespace types {

   using std::string;
   using std::vector;
   using std::set;
   using std::map;
   using std::pair;

   class types_constructor
   {
   public:

      static const map<string, type_id::builtin> known_builtin_types;

      types_constructor() {}

      types_constructor( const ABI& abi );

      void             forward_declare(const set<string>& forward_declarations);
      inline type_id::index_t forward_declare(const char* declared_name) { return forward_declare(string(declared_name)); }
      type_id::index_t forward_declare(const string& declared_name);
      type_id::index_t add_struct(const string& name, const vector<pair<type_id, int16_t>>& fields, type_id base = type_id(), int16_t base_sort = 0 );
      type_id::index_t add_tuple(const vector<type_id>& fields);
      type_id::index_t add_array(type_id element_type, uint32_t num_elements);
      type_id::index_t add_vector(type_id element_type);
      type_id::index_t add_optional(type_id element_type);
      type_id::index_t add_variant(const vector<type_id>& cases);
      type_id::index_t add_table(type_id::index_t object_index, const vector<ABI::table_index>& indices);

      bool             is_type_valid(type_id tid)const;
      inline bool      is_disabled()const { return disabled; }
      bool             is_complete()const;
      
      void             complete_size_align_cache();
      pair<types_manager, full_types_manager>  destructively_extract_types_managers();

   private:
      
      struct struct_view_entry
      {
         type_id::index_t object_index;
         type_id          key_type;
         vector<uint16_t> mapping;
      };

      vector<uint32_t>                  types_minimal;
      vector<field_metadata>            members_minimal;
      vector<uint32_t>                  types_full;
      vector<field_metadata>            members_full;
 
      bool                              disabled = false;

      type_id::index_t                  active_struct_index = type_id::type_index_limit;

      map<string,    type_id::index_t>                struct_lookup;
      map<vector<type_id>, type_id::index_t>          tuple_lookup;
      map<pair<type_id, uint32_t>, type_id::index_t>  array_lookup;
      map<type_id,   type_id::index_t>                vector_lookup;
      map<type_id,   type_id::index_t>                optional_lookup;
      map<vector<type_id>, type_id::index_t>          variant_lookup;
      map<type_id::index_t, type_id::index_t>         table_lookup;

      map<type_id::index_t, string>     valid_struct_start_indices;
      set<type_id::index_t>             valid_tuple_start_indices;
      set<type_id::index_t>             valid_array_start_indices;
      set<type_id::index_t>             valid_vector_start_indices;
      set<type_id::index_t>             valid_sum_type_start_indices;
      set<type_id::index_t>             valid_table_start_indices;

      flat_map<type_id::index_t, type_id> valid_indices;

      map<string, type_id::index_t>     incomplete_structs;

      uint32_t                          num_types_with_cacheable_size_align      = 0;
      mutable uint32_t                  num_types_with_cached_size_align_minimal = 0;
      mutable uint32_t                  num_types_with_cached_size_align_full    = 0;

      vector<struct_view_entry>         struct_views;

      void check_disabled()const;
      void name_conflict_check(const string& name, bool skip_structs = false)const;
 
      type_id          remap_abi_type(const ABI& abi, map<uint32_t, uint32_t>& index_map, map<uint32_t, set<string>>& struct_map, type_id tid);
      type_id::index_t process_abi_type(const ABI& abi, map<uint32_t, uint32_t>& index_map, map<uint32_t, set<string>>& struct_map, uint32_t i);
     
      type_id::index_t add_empty_struct_to_end();
      void             complete_struct(type_id::index_t index, const vector<pair<type_id, int16_t>>& fields,
                                       type_id base = type_id(), int16_t base_sort = 0,
                                       bool was_previously_declared = false);
      pair<type_id::size_align, uint16_t> process_struct(const vector<pair<type_id, int16_t>>& fields, type_id base, int16_t base_sort);

      pair<uint32_t, uint32_t> add_struct_view(type_id::index_t object_index, type_id::builtin builtin_type, uint16_t object_member_index);
      pair<uint32_t, uint32_t> add_struct_view(type_id::index_t object_index, type_id::index_t key_index, const vector<uint16_t>& mapping);

      type_id::size_align get_size_align(type_id tid)const;


      static inline uint8_t get_alignment_mask(uint8_t alignment) // Returns 0 if alignment is not a positive power of 2
      {
         if ( alignment == 0 || ((alignment & (alignment - 1)) != 0) ) // If alignment is not a positive power of 2
            return 0;
         return (alignment - 1);
      }

      static inline uint8_t bytes_until_next_alignment(uint32_t pos, uint8_t alignment_mask)
      {
         uint8_t bytes_after_alignment = static_cast<uint8_t>(pos) & alignment_mask;
         if( bytes_after_alignment == 0 )
            return 0;
         return (alignment_mask + 1 - bytes_after_alignment);
      }

      static inline uint8_t log2_of_power_of_2(uint32_t n) // Assumes n is a power of 2
      {
         // Adapted from: https://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
         static const uint8_t MultiplyDeBruijnBitPosition[32] = 
         {
            0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
            31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
         };

         return MultiplyDeBruijnBitPosition[((uint32_t)(n * 0x077CB531U)) >> 27];
        
      }

      static inline uint8_t count_zero_bits_on_right(uint32_t n)
      {
         // Adapted from: https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup

         return log2_of_power_of_2(n & -n); // Returns 0 if n == 0
      }

  };

} }

