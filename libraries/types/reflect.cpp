#include <eos/types/reflect.hpp>

#include <string>
#include <cstring>
#include <boost/container/flat_map.hpp>

namespace eos { namespace types {

   using std::string;
   using boost::container::flat_map;

   uint32_t register_struct(abi_constructor& ac, const std::vector<type_id>& field_types, const char* struct_name, 
                            const char* const field_names[], uint32_t field_names_length,
                            const char* const sorted_member_names[], bool const sorted_member_dir[], uint32_t sorted_member_count,
                            const char* base_name)
   {
      if( field_types.size() != field_names_length )
         throw std::runtime_error("Unexpected failure with type discovery visitor.");

      flat_map<string, uint16_t> field_name_map;
      field_name_map.reserve(field_names_length);
      
      vector<pair<string, type_id>> fields;
      fields.reserve(field_names_length);

      vector<int16_t> sort_order;
      sort_order.reserve(sorted_member_count);

      for( uint32_t i = 0; i < field_names_length; ++i )
      {
         field_name_map.emplace(field_names[i], i);
         fields.emplace_back(field_names[i], field_types[i]);
      }
      
      for( uint32_t i = 0; i < sorted_member_count; ++i )
      {
         string s(sorted_member_names[i]);

         if( base_name != nullptr && ( (base_name == sorted_member_names[i]) || (std::strcmp(base_name, sorted_member_names[i]) == 0) ) )
         {
            sort_order.push_back( (sorted_member_dir[i] ? 0 : -1) );
            continue;
         }

         auto itr = field_name_map.find(s);
         if( itr == field_name_map.end() )
            throw std::logic_error("Specified a name in the sort priority list that was not a field name.");

         sort_order.push_back( (sorted_member_dir[i] ? (itr->second + 1) : (-itr->second - 2)) );
      }

      return ac.add_struct(struct_name, fields, sort_order, base_name);
   }

} }

