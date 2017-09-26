#pragma once

#include <eos/types/types_manager_common.hpp>
#include <eos/types/reflect.hpp>
#include <eos/types/bit_view.hpp>

#include <iosfwd>
#include <string>
#include <boost/container/flat_map.hpp>

namespace eos { namespace types {

   using std::string;
   using boost::container::flat_map;

   class full_types_manager : public types_manager_common
   {
   public:
      
      using continuation_window  = bit_view<bool,     31,  1, uint32_t>;
      using index_type_window    = bit_view<uint8_t,  28,  3, uint32_t>;
      using three_bits_window    = bit_view<uint8_t,  24,  3, uint32_t>;
      using seven_bits_window    = bit_view<uint8_t,  24,  7, uint32_t>;
      using fifteen_bits_window  = bit_view<uint16_t, 16, 15, uint32_t>;
      using index_window         = bit_view<uint32_t,  0, 24, uint32_t>;
      using five_bits_window     = bit_view<uint8_t,  26,  5, uint32_t>;
      using large_index_window   = bit_view<uint32_t,  0, 26, uint32_t>;
      using field_size_window    = bit_view<uint16_t,  0, 16, uint32_t>;
      using base_sort_window     = bit_view<int16_t,   0, 16, uint32_t>;

      using sort_order_window    = bit_view<int16_t,  48, 16>;
      using fields_index_window  = bit_view<uint64_t,  0, 40>;

      enum class index_type : uint8_t
      {
         table_index = 0,
         simple_struct_index,
         derived_struct_index,
         tuple_index,
         array_index,
         vector_index,
         sum_type_index,
      };

      full_types_manager(const full_types_manager& other)
         : types_manager_common(types, members), 
           types(other.types), members(other.members), 
           lookup_by_name(other.lookup_by_name), valid_indices(other.valid_indices),
           field_names(other.field_names), fields_info(other.fields_info)
      {
      }

      full_types_manager(full_types_manager&& other)
         : types_manager_common(types, members), 
           types(std::move(other.types)), members(std::move(other.members)), 
           lookup_by_name(std::move(other.lookup_by_name)), valid_indices(std::move(other.valid_indices)),
           field_names(std::move(other.field_names)), fields_info(std::move(other.fields_info))
      {
      }

      range<vector<field_metadata>::const_iterator>      get_all_members(type_id::index_t struct_index)const;
      field_metadata                                     get_member(type_id::index_t struct_index, uint16_t member_index)const;

      type_id::index_t                                   get_table(const string& name)const;
      type_id::index_t                                   get_struct_index(const string& name)const;
      string                                             get_struct_name(type_id::index_t index)const;

      template<typename T>
      inline
      typename std::enable_if<eos::types::reflector<T>::is_struct::value, type_id::index_t>::type
      get_struct_index()const
      {
         return get_struct_index(eos::types::reflector<T>::name());
      }

      bool                                               is_type_valid(type_id tid)const;
      type_id::size_align                                get_size_align(type_id tid)const;

      void print_type(std::ostream& os, type_id tid)const;

      friend class types_constructor;
      friend struct print_type_visitor;

   private:
      
      vector<uint32_t>       types;
      vector<field_metadata> members;
      flat_map<string, uint32_t>  lookup_by_name; // Value is an index into valid_indices. 
      vector<uint32_t> valid_indices; // Entries can be multiple consecutive uint32_t's (4 if struct entry, 2 if table entry).
                                      // All entires contain the index (into types vector) for the type (or table), and entries in valid_indices are sorted by that index value.
                                      // Table also includes the number of indices (cached) and an index into valid_indices for the entry corresponding to the table object struct.
                                      // Struct also includes index into lookup_by_name (for struct name), total number of fields of the struct, the (signed) base sort order (if applicable),
                                      //   and an index into fields_info for the start (i.e. for first field) of the information for each of the fields of the struct in order.
      vector<string>   field_names;   
      vector<uint64_t> fields_info;   // The subset of consecutive uint64_t's for a specific struct contain the information for each of the fields (in order) of that struct.
                                      // Each uint64_t contains info for a particular field of a particular struct. It contains the (signed) field sort order as well as the 
                                      // index into field_names specifying the name of that field.

      full_types_manager(const vector<uint32_t>& t, const vector<field_metadata>& m, 
                         const flat_map<string, uint32_t>& lookup, 
                         const vector<uint32_t>& v_i,
                         const vector<string>& f_n, 
                         const vector<uint64_t>& f_i)
         : types_manager_common(types, members), types(t), members(m), 
           lookup_by_name(lookup), valid_indices(v_i), field_names(f_n), fields_info(f_i)
      {
      }

      full_types_manager(vector<uint32_t>&& t, vector<field_metadata>&& m, 
                         flat_map<string, uint32_t>&& lookup, 
                         vector<uint32_t>&& v_i,
                         vector<string>&& f_n,
                         vector<uint64_t>&& f_i) 
         : types_manager_common(types, members), types(std::move(t)), members(std::move(m)), 
           lookup_by_name(std::move(lookup)), valid_indices(std::move(v_i)), field_names(std::move(f_n)), fields_info(std::move(f_i))
      {
      }

      pair<const string&, int16_t> get_struct_info(vector<uint32_t>::const_iterator itr)const;
      range<vector<uint64_t>::const_iterator> get_struct_fields_info(vector<uint32_t>::const_iterator itr)const;

      std::ptrdiff_t adjust_iterator(vector<uint32_t>::const_iterator& itr)const;
      std::ptrdiff_t advance_iterator(vector<uint32_t>::const_iterator& itr)const;
      vector<uint32_t>::const_iterator find_index(type_id::index_t index)const;

   };

} }

