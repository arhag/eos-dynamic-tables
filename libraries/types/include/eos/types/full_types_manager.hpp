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
      
      using is_table_window = bit_view<bool,     31,  1, uint32_t>;
      using index_window    = bit_view<uint32_t,  0, 24, uint32_t>;

      full_types_manager(const full_types_manager& other)
         : types_manager_common(types, members), 
           types(other.types), members(other.members), 
           lookup_by_name(other.lookup_by_name), valid_indices(other.valid_indices)
      {
      }

      full_types_manager(full_types_manager&& other)
         : types_manager_common(types, members), 
           types(std::move(other.types)), members(std::move(other.members)), 
           lookup_by_name(std::move(other.lookup_by_name)), valid_indices(std::move(other.valid_indices))
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
      flat_map<string, uint32_t>  lookup_by_name;      
      flat_map<type_id::index_t, type_id> valid_indices;

      full_types_manager(const vector<uint32_t>& t, const vector<field_metadata>& m, 
                         const flat_map<string, uint32_t>& lookup, const flat_map<type_id::index_t, type_id>& v_i) 
         : types_manager_common(types, members), types(t), members(m), lookup_by_name(lookup), valid_indices(v_i) 
      {
      }

      full_types_manager(vector<uint32_t>&& t, vector<field_metadata>&& m, 
                         flat_map<string, uint32_t>&& lookup, flat_map<type_id::index_t, type_id>& v_i) 
         : types_manager_common(types, members), types(t), members(m), lookup_by_name(lookup), valid_indices(v_i)
      {
      }

   };

} }

