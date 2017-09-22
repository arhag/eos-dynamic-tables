#include <eos/types/abi_constructor.hpp>

#include <stdexcept>
#include <limits>

namespace eos { namespace types {

   type_id::index_t abi_constructor::declare_struct(const char* name)
   {
      auto res = struct_lookup.emplace(string(name), 0);
      if( !res.second )
         return res.first->second;
      
      auto index = abi.types.size();
      if( index >= type_id::type_index_limit )
         throw std::logic_error("Too many structs declared/defined.");

      abi.types.push_back({ .first = std::numeric_limits<uint32_t>::max(), .second = -1, .ts = ABI::struct_type });
      res.first->second = index;

      return index;
   }

   type_id::index_t abi_constructor::add_struct(const char* name_cstr, const vector<pair<string, type_id>>& fields, const vector<int16_t>& sort_order, const char* base_name)
   {
      auto name = string(name_cstr);
      auto itr = struct_lookup.find(name);

      type_id::index_t index;
      if( itr != struct_lookup.end() )
         index = declare_struct(name_cstr);
      else
         index = itr->second;

      if( abi.types[index].first != std::numeric_limits<uint32_t>::max() )
         throw std::logic_error("Struct with same name already added.");

      abi.types[index].first  = static_cast<uint32_t>(abi.structs.size());

      if( base_name != nullptr )
      {
         auto base_index = get_struct_index(base_name);
         if( base_index < 0 )
            throw std::logic_error("Base struct has not yet been added.");
         abi.types[index].second = base_index;
      }
  
      abi.structs.push_back({ .name = name, .fields = fields, .sort_order = sort_order });
      
      return index;
   }

   int32_t abi_constructor::get_struct_index(const char* name)const
   {
      auto itr = struct_lookup.find(string(name));
      if( itr == struct_lookup.end() )
         return -1;
      return static_cast<int32_t>(itr->second);
   }

   type_id::index_t abi_constructor::add_tuple(const vector<type_id>& fields)
   {
      if( fields.size() < 2 )
         throw std::invalid_argument("There must be at least 2 fields within the tuple.");

      auto itr = tuple_lookup.find(fields);
      if( itr != tuple_lookup.end() )
         return itr->second;

      if( abi.type_sequences.size() >= type_id::type_index_limit )
         throw std::logic_error("Too many variants or tuples defined.");

      type_id::index_t index = abi.types.size();
      abi.types.push_back({ .first = static_cast<uint32_t>(abi.type_sequences.size()), .second = static_cast<int32_t>(fields.size()), .ts = ABI::tuple_type });
      for( auto t : fields )
         abi.type_sequences.push_back(t);
      
      tuple_lookup.emplace(fields, index);
      return index;

   }

   type_id::index_t abi_constructor::add_array(type_id element_type, uint32_t num_elements)
   {
      if( num_elements < 2 )
         throw std::domain_error("Number of elements must be at least 2.");

      if( num_elements >= type_id::array_size_limit )
         throw std::domain_error("Number of elements is too large.");

      auto itr = array_lookup.find(std::make_pair(element_type, num_elements));
      if( itr != array_lookup.end() )
         return itr->second;

      type_id::index_t index = abi.types.size();
      abi.types.push_back({ .first = element_type.get_storage(), .second = static_cast<int32_t>(num_elements), .ts = ABI::array_type });

      array_lookup.emplace(std::make_pair(element_type, num_elements), index);
      return index;
   }

   type_id::index_t abi_constructor::add_vector(type_id element_type)
   {
      auto itr = vector_lookup.find(element_type);
      if( itr != vector_lookup.end() )
         return itr->second;

      type_id::index_t index = abi.types.size();
      abi.types.push_back({ .first = element_type.get_storage(), .second = -1, .ts = ABI::vector_type });

      vector_lookup.emplace(element_type, index);
      return index; 
   }

   type_id::index_t abi_constructor::add_optional(type_id element_type)
   {
      auto itr = optional_lookup.find(element_type);
      if( itr != optional_lookup.end() )
         return itr->second;

      type_id::index_t index = abi.types.size();
      abi.types.push_back({ .first = element_type.get_storage(), .second = -1, .ts = ABI::optional_type });

      optional_lookup.emplace(element_type, index);
      return index; 
  }

   type_id::index_t abi_constructor::add_variant(const vector<type_id>& cases)
   {
      if( cases.size() < 2 )
         throw std::invalid_argument("There must be at least 2 cases within the variant.");

      if( cases.size() >= type_id::variant_case_limit )
         throw std::invalid_argument("There are too many cases within the variant.");

      auto itr = variant_lookup.find(cases);
      if( itr != variant_lookup.end() )
         return itr->second;

      if( abi.type_sequences.size() >= type_id::type_index_limit )
         throw std::logic_error("Too many variants or tuples defined.");

      type_id::index_t index = abi.types.size();
      abi.types.push_back({ .first = static_cast<uint32_t>(abi.type_sequences.size()), .second = static_cast<int32_t>(cases.size()), .ts = ABI::variant_type });
      for( auto t : cases )
         abi.type_sequences.push_back(t);
      
      variant_lookup.emplace(cases, index);
      return index;
   }


   uint32_t abi_constructor::add_table(const char* object_name, const vector<ABI::table_index>& indices)
   {
      auto object_index = get_struct_index(object_name);
      if( object_index == -1 )
         throw std::invalid_argument("No struct found with the given name.");

      abi.tables.push_back(ABI::table{ .object_index = static_cast<type_id::index_t>(object_index), .indices = indices });

      return (abi.tables.size() - 1);
   }

} }

