#pragma once

#include <eos/types/types_manager_common.hpp>

#include <string>
#include <boost/container/flat_map.hpp>

namespace eos { namespace types {

   using std::string;
   using boost::container::flat_map;

   class types_manager : public types_manager_common
   {
   public:
 
      types_manager(const types_manager& other)
         : types_manager_common(types, members), 
           types(other.types), members(other.members), 
           table_lookup(other.table_lookup)
      {
      }
     
      types_manager(types_manager&& other)
         : types_manager_common(types, members), 
           types(std::move(other.types)), members(std::move(other.members)), 
           table_lookup(std::move(other.table_lookup))
      {
      }
      
      type_id::index_t                                   get_table(const string& name)const;

      friend class types_constructor;

   private:
      
      vector<uint32_t>       types;
      vector<field_metadata> members;
      flat_map<string, type_id::index_t> table_lookup;      

      types_manager(const vector<uint32_t>& t, const vector<field_metadata>& m, const flat_map<string, type_id::index_t>& tbl_lookup) 
         : types_manager_common(types, members), types(t), members(m), table_lookup(tbl_lookup) 
      {
      }

      types_manager(vector<uint32_t>&& t, vector<field_metadata>&& m, flat_map<string, type_id::index_t>&& tbl_lookup) 
         : types_manager_common(types, members), types(t), members(m), table_lookup(tbl_lookup) 
      {
      }

   };

} }

