#include <eos/types/types_manager.hpp>

namespace eos { namespace types {

   type_id::index_t types_manager::get_table(const string& name)const
   {
      auto itr = table_lookup.find(name);
      if( itr == table_lookup.end() )
         throw std::invalid_argument("Cannot find table with the given name.");

      return itr->second;
   }

} }

