#pragma once

#include <eos/types/abi_definition.hpp>
#include <eos/types/reflect.hpp>

EOS_TYPES_REFLECT_BUILTIN( eos::types::ABI::type_specification, builtin_uint8 )
EOS_TYPES_REFLECT_STRUCT( eos::types::ABI::type_definition, (first)(second)(ts) )
EOS_TYPES_REFLECT_STRUCT( eos::types::ABI::struct_t, (name)(fields)(sort_order) )
EOS_TYPES_REFLECT_STRUCT( eos::types::ABI::table_index, (key_type)(unique)(ascending)(mapping) )
EOS_TYPES_REFLECT_STRUCT( eos::types::ABI::table, (object_index)(indices) )
EOS_TYPES_REFLECT_STRUCT( eos::types::ABI, (types)(structs)(type_sequences)(tables) )
