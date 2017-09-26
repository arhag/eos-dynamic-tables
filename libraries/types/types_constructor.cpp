#include <eos/types/types_constructor.hpp>
#include <eos/types/bit_view.hpp>
#include <eos/types/types_manager_common.hpp>

#include <stdexcept>
#include <algorithm>
#include <tuple>
#include <limits>
#include <cctype>
#include <boost/container/flat_map.hpp>

namespace eos { namespace types {

   using boost::container::flat_map;
   using std::tuple;

   const map<string, type_id::builtin> types_constructor::known_builtin_types =
      { {"UInt8",    type_id::builtin_uint8   },
        {"Int8",     type_id::builtin_int8    },
        {"UInt16",   type_id::builtin_uint16  },
        {"Int16",    type_id::builtin_int16   },
        {"UInt32",   type_id::builtin_uint32  },
        {"Int32",    type_id::builtin_int32   },
        {"UInt64",   type_id::builtin_uint64  },
        {"Int64",    type_id::builtin_int64   },
        {"Bytes",    type_id::builtin_bytes   },
        {"String",   type_id::builtin_string  },
        {"Bool",     type_id::builtin_bool    },
        {"Rational", type_id::builtin_rational},
        {"Any",      type_id::builtin_any     }
      };

   bool is_name_valid(const string& name)
   {
      if( name.size() == 0 )
         return false;

      bool first = true;
      bool expectingColon = false;
      bool disallowColon = false;
      for( auto c : name )
      {
         if( first )
         {
            first = false;
            if( !std::isalpha(c) && c != '_' && (c != ':' || disallowColon) )
               return false;
         }
         else if( !std::isalnum(c) && c != '_' && (c != ':' || disallowColon) )
            return false;

         if( c == ':' )
         {
            if( expectingColon ) // Continuation of :: seperator
            {
               expectingColon = false;
               disallowColon = true; // Make sure more than two consecutive colons are not allowed
            }
            else // Start of :: seperator
               expectingColon = true;
         }
         else if( expectingColon )
            return false; // Cannot have a single : as separator
         else
            disallowColon = false; // Reset consecutive colon counter
      }

      if( expectingColon || disallowColon ) // Do not allow any trailing colons
         return false;

      return true; 
   } 

   type_id::index_t types_constructor::process_abi_type(const ABI& abi, map<uint32_t, uint32_t>& index_map, map<uint32_t, set<string>>& struct_map, uint32_t i)
   {
      const auto& t = abi.types[i];

      auto index_itr = index_map.find(i);
      if( index_itr != index_map.end() )
         return index_itr->second;

      switch( t.ts )
      {
         case ABI::struct_type:
         {
            const auto& s = abi.structs.at(t.first);

            if( !is_name_valid(s.name) )
               throw std::invalid_argument("Name of struct is not valid.");

            index_map.emplace(i, forward_declare(s.name));

            auto itr = struct_map.find(i);
            if( itr == struct_map.end() )
            {
               auto res = struct_map.emplace(i, set<string>());
               itr = res.first;
            }
            auto& names_of_fields = itr->second;

            type_id base;
            if( t.second != -1 )
            {
               auto base_tid = type_id::make_struct(static_cast<type_id::index_t>(t.second));
               base = remap_abi_type(abi, index_map, struct_map, base_tid);
            }

            uint64_t struct_fields_index = struct_fields.size();
            struct_fields.resize( struct_fields.size() + s.fields.size() );

            vector<pair<type_id, int16_t>> fields;
            uint64_t indx = struct_fields_index;
            for( const auto& p : s.fields )
            {
               if( !is_name_valid(p.first) )
                  throw std::invalid_argument("Name of field is not valid.");

               if( !names_of_fields.insert(p.first).second )
                  throw std::invalid_argument("Another field with same name already exists within the struct.");

               auto res = field_names.emplace(p.first, 0);
               struct_fields[indx].first = res.first;

               auto t = remap_abi_type(abi, index_map, struct_map, p.second);
               fields.emplace_back(t, 0);

               ++indx;
            }

            int16_t base_sort = 0;
            uint16_t counter = 1;
            for( auto j : s.sort_order )
            {
               if( j == 0 )
                  base_sort = counter;
               else if( j == -1 )
                  base_sort = -counter;
               else if( j > 0 )
               {
                  fields.at(j-1).second = counter;
                  struct_fields[struct_fields_index + (j-1)].second = counter;
               }
               else
               {
                  fields.at(-(j+2)).second = -counter;
                  struct_fields[struct_fields_index - (j+2)].second = -counter;
               }
               ++counter;
            }

            auto index = add_struct(s.name, fields, base, base_sort);
            uint64_t storage = 0;
            full_types_manager::fields_index_window::set(storage, struct_fields_index);
            full_types_manager::sort_order_window::set(storage, base_sort);
            struct_fields_map.emplace(index, storage);
            return index;
         }
         case ABI::tuple_type:
         {
            uint32_t num_fields = static_cast<uint32_t>(t.second);
            if( (t.first + num_fields) >  abi.type_sequences.size() )
               throw std::out_of_range("Invalid tuple specification.");

            vector<type_id> fields;
            fields.reserve(num_fields);
            auto itr = abi.type_sequences.begin() + t.first;
            for( uint32_t j = 0; j < num_fields; ++j, ++itr )
            {
               auto t = remap_abi_type(abi, index_map, struct_map, *itr);
               fields.push_back(t);                 
            }
         
            auto index = add_tuple(fields);
            struct_map.emplace(i, set<string>());
            index_map.emplace(i, index);
            return index;
         }
         case ABI::array_type:
         {
            auto index = add_array(remap_abi_type(abi, index_map, struct_map, type_id(t.first)), t.second); 
            index_map.emplace(i, index);
            return index;
         }
         case ABI::vector_type:
         {
            auto index = add_vector(remap_abi_type(abi, index_map, struct_map, type_id(t.first)));
            index_map.emplace(i, index);
            return index;
         }
         case ABI::optional_type: 
         {
            auto index = add_optional(remap_abi_type(abi, index_map, struct_map, type_id(t.first)));
            index_map.emplace(i, index);
            return index;
         }
         case ABI::variant_type:
         {
            uint32_t num_cases = static_cast<uint32_t>(t.second);
            if( num_cases >= type_id::variant_case_limit )
               throw std::invalid_argument("Variant has too many cases.");
            if( (t.first + num_cases) >  abi.type_sequences.size() )
               throw std::out_of_range("Invalid variant specification.");

            vector<type_id> cases;
            cases.reserve(num_cases);
            auto itr = abi.type_sequences.begin() + t.first;
            for( uint32_t j = 0; j < num_cases; ++j, ++itr )
            {
               auto t = remap_abi_type(abi, index_map, struct_map, *itr);
               cases.push_back(t);                 
            }

            auto index = add_variant(cases);
            index_map.emplace(i, index);
            return index;
         }
      }

      return type_id::type_index_limit; // Should never get here, but added to silence compiler.
   }

   type_id types_constructor::remap_abi_type(const ABI& abi, map<uint32_t, uint32_t>& index_map, map<uint32_t, set<string>>& struct_map, type_id tid)
   {
      switch( tid.get_type_class() )
      {
         case type_id::builtin_type:
         case type_id::small_array_of_builtins_type:
            return tid;
         case type_id::vector_of_something_type:
            if( tid.get_element_type().get_type_class() != type_id::struct_type )
               return tid; // Must be a vector of array of builtins or vector of builtins
         case type_id::struct_type:
         case type_id::array_type:
         case type_id::small_array_type:
         case type_id::vector_type:
         case type_id::optional_struct_type:
         case type_id::variant_or_optional_type:
            break;
      }

      auto i = tid.get_type_index();
      auto itr = index_map.find(i);
      type_id::index_t index;
      if( itr == index_map.end() )
            index = process_abi_type(abi, index_map, struct_map, i);
      else
         index = itr->second;
      tid.set_type_index(index);
      return tid;
   }

   types_constructor::types_constructor( const ABI& abi )
   {
      map<uint32_t, uint32_t>    index_map;
      map<uint32_t, set<string>> struct_map;

      if( abi.types.size() == 0 )
         throw std::invalid_argument("ABI must define at least one type.");

      for( uint32_t i = 0; i < abi.types.size(); ++i )
      {
         if( abi.types[i].ts != ABI::struct_type )
            continue;

         struct_map.emplace(i, set<string>());
         process_abi_type(abi, index_map, struct_map, i);
      } 

      if( index_map.size() != abi.types.size() )
         throw std::logic_error("Unnecessary non-struct types were included in the ABI.");

      set<uint32_t> index_set;
      for( const auto& p : index_map )
      {
         auto res = index_set.insert(p.second);
         if( !res.second )
            throw std::logic_error("At least two non-struct types of identical definition found in ABI.");
      }

      for( const auto& tbl : abi.tables )
      {
         if( struct_map.find(tbl.object_index) == struct_map.end() )
            throw std::invalid_argument("Table object refers either to an invalid index or to an index of a non-struct type.");

         auto object_index = remap_abi_type(abi, index_map, struct_map, type_id::make_struct(tbl.object_index)).get_type_index();
         vector<ABI::table_index> indices;
         indices.reserve(tbl.indices.size());

         for( const auto& indx : tbl.indices )
         {
            indices.push_back(indx);
            
            auto tc = indx.key_type.get_type_class();
            if( tc == type_id::builtin_type )
               continue;

            if( tc != type_id::struct_type )
               throw std::invalid_argument("Table index keys can only be either builtin types or product types (structs or tuples).");

            auto key_index = indx.key_type.get_type_index();
            if( struct_map.find(key_index) == struct_map.end()  )
               throw std::invalid_argument("Table index key refers either to an invalid index or to an index of type that is neither a struct or a tuple.");

            indices.back().key_type = remap_abi_type(abi, index_map, struct_map, type_id::make_struct(key_index));
         }

         add_table(object_index, indices);
      }
   }

   void types_constructor::check_disabled()const
   {
      if( disabled )
         throw std::logic_error("This types_constructor is disabled.");
   }

   void types_constructor::name_conflict_check(const string& name, bool skip_structs)const
   {
      if( name == string() )
         throw std::invalid_argument("Name cannot be blank.");

      if( known_builtin_types.find(name) != known_builtin_types.end() )
         throw std::invalid_argument("Already have type with same name in builtin types.");

      if( !skip_structs )
         if( struct_lookup.find(name) != struct_lookup.end() )
            throw std::invalid_argument("Already added struct with same name.");
   }

   void types_constructor::forward_declare(const set<string>& forward_declarations)
   {
      check_disabled();

      for( const string& declared_name : forward_declarations )
      {
         if( struct_lookup.find(declared_name) != struct_lookup.end() )
            continue;
         if( incomplete_structs.find(declared_name) != incomplete_structs.end() )
            continue;
         name_conflict_check(declared_name, true);

         auto index = add_empty_struct_to_end();
         incomplete_structs.emplace(declared_name, index);
         valid_struct_start_indices.emplace(index, declared_name); 
         ++num_types_with_cacheable_size_align;
      }
   }

   type_id::index_t types_constructor::forward_declare(const string& declared_name)
   {
      check_disabled();
      
      auto itr1 = struct_lookup.find(declared_name);
      if( itr1 != struct_lookup.end() )
         return itr1->second;

      auto itr2 = incomplete_structs.find(declared_name);
      if( itr2  != incomplete_structs.end() )
         return itr2->second;

      name_conflict_check(declared_name, true);

      auto index = add_empty_struct_to_end();
      incomplete_structs.emplace(declared_name, index);
      valid_struct_start_indices.emplace(index, declared_name); 
      ++num_types_with_cacheable_size_align;

      return index;
   }

   type_id::index_t types_constructor::add_empty_struct_to_end()
   {
      type_id::index_t index = types_full.size();
      if( types_minimal.size() != index )
         throw std::runtime_error("Invariant failure: types_full and types_minimal vectors are out of sync.");

      types_minimal.push_back(type_id::size_align().get_storage());
      types_minimal.push_back(0);
      types_minimal.push_back(0);
      types_minimal.push_back(0);

      types_full.push_back(type_id::size_align().get_storage());
      types_full.push_back(0);
      types_full.push_back(0);
      types_full.push_back(0);
 
      return index;
   }

   void types_constructor::complete_struct(type_id::index_t index,
                                           const vector<pair<type_id, int16_t>>& fields,
                                           type_id base, int16_t base_sort,
                                           bool was_previously_declared)
   {
      using sorted_window    = types_manager_common::sorted_window; 
      using ascending_window = types_manager_common::ascending_window;
      using index_window     = types_manager_common::index_window;



      active_struct_index = index;
      uint32_t original_minimal_members_size = members_minimal.size();
      uint32_t original_full_members_size    = members_full.size();
      try 
      {  
         type_id::size_align sa;
         uint16_t num_sorted_members = 0;
         std::tie(sa, num_sorted_members) = process_struct(fields, base, base_sort); // Actually adds field_metadata to end of members
         // If anything (meaning the process_struct function) throws an exception up to this point from the start of the try block,
         // then this function will be able to recover the state prior to being called.
        
         uint16_t total_num_members =  (base.is_void() ? fields.size() : fields.size() + 1);

         {
            auto itr = types_minimal.begin() + index;
            *itr = sa.get_storage();
            ++itr;
            *itr = original_minimal_members_size;
            ++itr;
            *itr = num_sorted_members;
            ++itr;
            uint32_t storage = 0;
            if( base.is_void() )
               index_window::set(storage, type_id::type_index_limit);
            else
            {
               sorted_window::set(storage, (base_sort != 0));
               ascending_window::set(storage, (base_sort > 0));
                //depth_window::set(storage, depth);
               index_window::set(storage, base.get_type_index());
            }
            *itr = storage;
         } 

         {
            auto itr = types_full.begin() + index;
            *itr = sa.get_storage();
            ++itr;
            *itr = original_full_members_size;
            ++itr;
            *itr = ( (total_num_members << 16) | num_sorted_members );
            ++itr;
            uint32_t storage = 0;
            if( base.is_void() )
               index_window::set(storage, type_id::type_index_limit);
            else
            {
               sorted_window::set(storage, (base_sort != 0));
               ascending_window::set(storage, (base_sort > 0));
                //depth_window::set(storage, depth);
               index_window::set(storage, base.get_type_index());
            }
            *itr = storage;
         } 

      }
      catch( ... ) 
      {
         active_struct_index = type_id::type_index_limit;
         if( !was_previously_declared )
         {
            types_minimal.resize(index);
            types_full.resize(index);
         }
         members_minimal.resize(original_minimal_members_size);
         members_full.resize(original_full_members_size);
         throw;
      }
      active_struct_index = type_id::type_index_limit;
   }

   type_id::index_t types_constructor::add_struct(const string& name, const vector<pair<type_id, int16_t>>& fields, type_id base, int16_t base_sort)
   {
      check_disabled();

      name_conflict_check(name);

      if( base.is_void() )
      {
         if( base_sort != 0 )
            throw std::invalid_argument("Cannot specify a sort order on a base when the struct does not have a base.");

         if( fields.size() == 0 )
            throw std::invalid_argument("A struct that does not inherit from a base must specify at least one field.");
      }
      else
      {
         if( base.get_type_class() != type_id::struct_type )
            throw std::invalid_argument("Base, if specified, can only be a struct.");
   
         if( !is_type_valid(base) )
            throw std::invalid_argument("Base type is invalid.");
      }

      if( fields.size() >= type_id::struct_fields_limit )
        throw std::invalid_argument("Struct has too many fields."); 

      auto incomplete_itr = incomplete_structs.find(name);
      bool was_previously_declared = (incomplete_itr != incomplete_structs.end());

      if( !was_previously_declared && (types_full.size() >= type_id::type_index_limit) )
         throw std::logic_error("Not enough room to add a struct with valid index."); 

      if( (members_full.size() + 2 + 2 * fields.size()) > std::numeric_limits<uint32_t>::max() ) 
         throw std::logic_error("Members vector could become too large.");

      type_id::index_t index = ( was_previously_declared ? incomplete_itr->second : add_empty_struct_to_end() );     
      complete_struct(index, fields,  base, base_sort, was_previously_declared); 

      struct_lookup.emplace(name, index);
      valid_struct_start_indices.emplace(index, name); 
      if( was_previously_declared )
         incomplete_structs.erase(incomplete_itr);
      else
         ++num_types_with_cacheable_size_align;
      ++num_types_with_cached_size_align_minimal;
      ++num_types_with_cached_size_align_full;
      return index;
   }

   type_id::index_t types_constructor::add_tuple(const vector<type_id>& field_types)
   {
      check_disabled();

      if( field_types.size() < 2 )
         throw std::invalid_argument("There must be at least 2 fields within the tuple.");
      
      if( field_types.size() >= type_id::struct_fields_limit )
         throw std::invalid_argument("There are too many fields within the tuple.");

      auto itr = tuple_lookup.find(field_types);
      if( itr != tuple_lookup.end() )
         return itr->second;

      if( types_full.size() >= type_id::type_index_limit)
         throw std::logic_error("Not enough room to add a tuple with valid index.");

      if( (members_full.size() + 2 * field_types.size()) > std::numeric_limits<uint32_t>::max() )
         throw std::logic_error("Members vector would become too large.");

      vector<pair<type_id, int16_t>> fields;
      fields.reserve(field_types.size());
      int16_t counter = 1;
      for( auto t : field_types )
      {
         if( !is_type_valid(t) )
            throw std::invalid_argument("Invalid type within tuple that was to be added.");
         fields.emplace_back(t, counter);
         ++counter;
      }

      type_id::index_t index = add_empty_struct_to_end();
      complete_struct(index, fields);

      tuple_lookup.emplace(field_types, index);
      valid_tuple_start_indices.insert(index);
      ++num_types_with_cacheable_size_align;
      ++num_types_with_cached_size_align_minimal;
      ++num_types_with_cached_size_align_full;
      return index;
   }

   pair<type_id::size_align, uint16_t> 
   types_constructor::process_struct(const vector<pair<type_id, int16_t>>& fields, type_id base, int16_t base_sort)
   {
      uint32_t size  = 0;
      uint8_t  align = 1;

      vector<field_metadata> member_metadata;

      vector<std::pair<type_id::size_align, uint16_t>> field_align_size_seq;
      field_align_size_seq.reserve(fields.size()); 

      // fobso_map is a map tracking sort orders for either fields or the base:
      // Key is the (absolute value of the) sort sequence number.
      // Value is the index into the member_metadata vector.
      flat_map<uint16_t, uint16_t> fobso_map;
      fobso_map.reserve(fields.size() + 1);


      bool base_exists = false;
      if( !base.is_void() ) // If base type exists
      {
         field_metadata::sort_order base_sort_order = ( (base_sort == 0) ? field_metadata::unsorted 
                                                                         : ( (base_sort > 0) ? field_metadata::ascending
                                                                                             : field_metadata::descending ) );

         base_exists = true;
         member_metadata.reserve(fields.size() + 1);

         auto sa = get_size_align(base);
         if( !sa.is_complete() )
            throw std::logic_error("Cannot inherit incomplete type.");

         member_metadata.emplace_back(base, 0, base_sort_order);

         if( base_sort_order != field_metadata::unsorted )
            fobso_map.emplace(std::abs(base_sort), 0);
         
         align = std::max(align, sa.get_align());
         size += sa.get_size();
      }
      else
      {
         member_metadata.reserve(fields.size());
      }


      // First pass through fields (in order specified by the fields vector):

      for( const auto& p :  fields )
      {
         uint8_t field_seq_num = field_align_size_seq.size();
      
         if( !is_type_valid(p.first) )
            throw std::invalid_argument("Field " + std::to_string(field_seq_num) + " has an invalid type.");

         field_metadata::sort_order field_sort_order = ( (p.second == 0) ? field_metadata::unsorted 
                                                                         : ( (p.second > 0) ? field_metadata::ascending
                                                                                            : field_metadata::descending ) );
         
         auto sa = get_size_align(p.first);
         if( !sa.is_complete() )
            throw std::logic_error("Incomplete type not allowed for field " + std::to_string(field_seq_num));

         field_align_size_seq.emplace_back(sa, field_seq_num); 
         member_metadata.emplace_back(p.first, 0, field_sort_order); // Set offset to 0 for now and fix later

         if( p.second != 0 )
         {
            auto temp = field_seq_num;
            if( base_exists )
               ++temp;

            auto res = fobso_map.emplace(std::abs(p.second), temp);
            if( !res.second )
               throw std::logic_error("Duplicate absolute value of sort order found in Struct specification.");
         }

         align = std::max(align, sa.get_align()); 
      }

      // Second pass through fields:
      // This time in order of descending alignment, descending size, and ascending field sequence number (in that order of priority),
      // in order to correct the offsets in member_metadata.
      // However, alignments of 0 or 1 are treated the same.
      // Due to the comparison definition of size_align, this is equivalent to sorting 
      // in order of the pair (size_align, field sequence number) in ascending order.
      std::sort(field_align_size_seq.begin(), field_align_size_seq.end()); 

      uint32_t end = size;
      vector<uint32_t> free_spaces; // Tracks free spaces in region up to (but not including) the current end.
      free_spaces.reserve(fields.size());

      using align_log2_window = bit_view<uint8_t, 29,  3, uint32_t>; // If disabled set to align_log2_window::all_ones (or 7), otherwise valid values are in the range [0, 5].
      using num_bytes_window  = bit_view<uint8_t, 24,  5, uint32_t>;
      using offset_window     = bit_view<uint32_t, 0, 24, uint32_t>;

      typename decltype(free_spaces)::difference_type free_space_begin_indx = 0;
      uint8_t alignment   = align;
      auto sa = field_align_size_seq.front().first;
      if( alignment > 32 || sa.get_align() != alignment )
         throw std::logic_error("Something has gone wrong with setting alignment. alignment = " + std::to_string(alignment) 
                                + ", alignment of first field to be laid out = " + std::to_string(sa.get_align()));
      uint8_t alignment_log2  = log2_of_power_of_2(alignment); 
      auto alignment_mask = get_alignment_mask(alignment); 
      for( const auto& t : field_align_size_seq )
      {
         auto member_index = ( base_exists ? t.second + 1 : t.second );
         auto total_size    = t.first.get_size();
         auto cur_alignment = t.first.get_align();
         if( (cur_alignment == 0 && alignment != 1) || alignment != cur_alignment )
         {
            // Switch to next alignment
            alignment      = (cur_alignment == 0 ? 1 : cur_alignment); // Handle special case of bool or bool array
            alignment_log2 = log2_of_power_of_2(alignment);
            alignment_mask = get_alignment_mask(alignment);
         }
         if( cur_alignment == 0 )
         {
            // For now I will do a less sophisticated algorithm of putting a bool into a byte and a bool array in sequence of bytes rounded to the nearest byte
            total_size = (total_size + 7)/8;
         }
         // At this point total_size should always be >= alignment and total_size is always in bytes

         auto b = bytes_until_next_alignment(end, alignment_mask); // b will always be less than alignment, thus b will always be less than total_size
         auto offset = end;

         bool no_room_before_end = true;

         // TODO: Write faster implementation?
         bool all_disabled_so_far = true;
         auto remainder = total_size % alignment;
         uint32_t total_size_rounded = total_size; // Total size rounded up to nearest multiple of alignment
         if( remainder != 0)
            total_size_rounded += alignment - remainder;
         for(auto itr = free_spaces.begin() + free_space_begin_indx; itr != free_spaces.end(); ++itr)
         {
            if( align_log2_window::get(*itr) == align_log2_window::all_ones ) // Signals that this free_space entry is disabled
               continue;
            if( all_disabled_so_far && itr - free_spaces.begin() != free_space_begin_indx )
               free_space_begin_indx = itr - free_spaces.begin();
            all_disabled_so_far = false;

            uint8_t bytes_left = num_bytes_window::get(*itr);
            auto free_space_start_offset = offset_window::get(*itr);
            auto b2 = bytes_until_next_alignment(free_space_start_offset, alignment_mask);

            if( static_cast<uint32_t>(bytes_left) < total_size_rounded + b2 )
               continue;

            // At this point we know we can fit the field before the end.
            no_room_before_end = false;

            offset = free_space_start_offset + bytes_left - total_size_rounded;
            
            // We just need to figure out how to update the free_space entry

            // Warning: For the sake of avoiding costly free_space middle insertions, 
            //          we are guaranteed to waste (total_size_rounded - total_size) bytes with this algorithm (unless b2 == 0).

            if( free_space_start_offset == offset )
            {
               if( total_size == bytes_left )
               {
                  // Disable the current free_space entry
                  if( itr - free_spaces.begin() == free_space_begin_indx )
                     ++free_space_begin_indx;
                  align_log2_window::set(*itr, align_log2_window::all_ones);
               }
               else
               {
                  offset_window::set(*itr, offset + total_size);
                  num_bytes_window::set(*itr, bytes_left - total_size);
               }
            }
            else
            {
               // This is the condition in which we guarantee wasting (total_size_rounded - total_size) bytes.
               num_bytes_window::set(*itr, offset - free_space_start_offset);
               align_log2_window::set(*itr, alignment_log2);
            }

         }
         if( all_disabled_so_far )
         {
            free_spaces.clear();
            free_space_begin_indx = 0;
         }

         if( no_room_before_end )
         {
            if( b != 0 )
            {
               uint32_t free_space = offset;
               num_bytes_window::set(free_space, b);
               align_log2_window::set(free_space, alignment_log2);;
               free_spaces.push_back(free_space);
               offset += b;
            }
            end = offset + total_size;
         }

         member_metadata[member_index].set_offset(offset);
         if( end > field_metadata::offset_limit )
            throw std::logic_error("Struct is too large.");
      }

      //size = end;
      size = end + bytes_until_next_alignment(end, get_alignment_mask(align));


      // Validate user specified sort orders and initialize sort_member_metadata:
      
      //sort_order.reserve(fobso_map.size());
      uint8_t expected_sort_order = 1;
      for( const auto& p : fobso_map )
      {
         if( p.first != expected_sort_order )
            throw std::logic_error("Sort orders are invalid.");

         ++expected_sort_order;

         members_minimal.push_back(member_metadata[p.second]);
         members_full.push_back(member_metadata[p.second]);
         //sort_order.push_back(p.second);
      }

      for( auto f : member_metadata )
         members_full.push_back(f);

      return {type_id::size_align(size, align), fobso_map.size()};
   }

   type_id::index_t types_constructor::add_array(type_id element_type, uint32_t num_elements)
   {
      check_disabled();

      if( num_elements < 2 )
         throw std::domain_error("Number of elements must be at least 2.");

      if( num_elements >= type_id::array_size_limit )
         throw std::domain_error("Number of elements is too large.");

      auto itr = array_lookup.find(std::make_pair(element_type, num_elements));
      if( itr != array_lookup.end() )
         return itr->second;

      if( types_full.size() >= type_id::type_index_limit)
         throw std::logic_error("Not enough room to add an array with valid index.");

      if( !is_type_valid(element_type) )
         throw std::invalid_argument("Invalid element type for adding an array.");

      type_id::index_t index = types_full.size();
      if( types_minimal.size() != index )
         throw std::runtime_error("Invariant failure: types_full and types_minimal vectors are out of sync.");

      types_minimal.push_back(type_id::size_align().get_storage());
      types_minimal.push_back(num_elements);
      types_minimal.push_back(element_type.get_storage());

      types_full.push_back(type_id::size_align().get_storage());
      types_full.push_back(num_elements);
      types_full.push_back(element_type.get_storage());

      array_lookup.emplace(std::make_pair(element_type, num_elements), index);
      valid_array_start_indices.insert(index);
      ++num_types_with_cacheable_size_align;
      return index;
   }

   type_id::index_t types_constructor::add_vector(type_id element_type)
   {
      check_disabled();

      auto itr = vector_lookup.find(element_type);
      if( itr != vector_lookup.end() )
         return itr->second;

      if( types_full.size() >= type_id::type_index_limit)
         throw std::logic_error("Not enough room to add a vector with valid index.");

      if( !is_type_valid(element_type) )
         throw std::invalid_argument("Invalid element type for adding a vector.");

      type_id::index_t index = types_full.size();
      if( types_minimal.size() != index )
         throw std::runtime_error("Invariant failure: types_full and types_minimal vectors are out of sync.");

      types_minimal.push_back(element_type.get_storage());
      types_full.push_back(element_type.get_storage());

      vector_lookup.emplace(element_type, index);
      valid_vector_start_indices.insert(index);
      return index; 
   }

   type_id::index_t types_constructor::add_optional(type_id element_type)
   {
      check_disabled();

      auto itr = optional_lookup.find(element_type);
      if( itr != optional_lookup.end() )
         return itr->second;

      if( types_full.size() >= type_id::type_index_limit)
         throw std::logic_error("Not enough room to add an optional with valid index.");

      if( !is_type_valid(element_type) )
         throw std::invalid_argument("Invalid element type for adding an optional.");

      type_id::index_t index = types_full.size();
      if( types_minimal.size() != index )
         throw std::runtime_error("Invariant failure: types_full and types_minimal vectors are out of sync.");

      types_minimal.push_back(type_id::size_align().get_storage());
      types_minimal.push_back(element_type.get_storage());

      types_full.push_back(type_id::size_align().get_storage());
      types_full.push_back(element_type.get_storage());

      optional_lookup.emplace(element_type, index);
      valid_sum_type_start_indices.insert(index);
      ++num_types_with_cacheable_size_align;
      return index; 
  }

   type_id::index_t types_constructor::add_variant(const vector<type_id>& cases)
   {
      check_disabled();

      if( cases.size() < 2 )
         throw std::invalid_argument("There must be at least 2 cases within the variant.");

      if( cases.size() >= type_id::variant_case_limit )
         throw std::invalid_argument("There are too many cases within the variant.");

      auto itr = variant_lookup.find(cases);
      if( itr != variant_lookup.end() )
         return itr->second;

      if( types_full.size() >= type_id::type_index_limit)
         throw std::logic_error("Not enough room to add a variant with valid index.");

      for( auto t : cases )
         if( !is_type_valid(t) )
            throw std::invalid_argument("Invalid type within variant that was to be added.");

      type_id::index_t index = types_full.size();
      if( types_minimal.size() != index )
         throw std::runtime_error("Invariant failure: types_full and types_minimal vectors are out of sync.");

      types_minimal.push_back(type_id::size_align().get_storage());
      types_minimal.push_back(static_cast<uint16_t>(cases.size()));
      for( auto t : cases )
         types_minimal.push_back(t.get_storage());
 
      types_full.push_back(type_id::size_align().get_storage());
      types_full.push_back(static_cast<uint16_t>(cases.size()));
      for( auto t : cases )
         types_full.push_back(t.get_storage());
     
      variant_lookup.emplace(cases, index);
      valid_sum_type_start_indices.insert(index);
      ++num_types_with_cacheable_size_align;
      return index;
   }

   type_id::index_t types_constructor::add_table(type_id::index_t object_index, const vector<ABI::table_index>& indices)
   {
      check_disabled();
      
      using unique_window     = types_manager_common::unique_window;
      using ascending_window  = types_manager_common::ascending_window;
      using index_type_window = types_manager_common::index_type_window;
      using index_window      = types_manager_common::index_window;

      auto itr = table_lookup.find(object_index);
      if( itr != table_lookup.end() )
         throw std::logic_error("Table already exists for the specified object.");   

      if( indices.size() >= 256 )
         throw std::invalid_argument("Tables cannot support that many indices.");

      if( !is_type_valid(type_id::make_struct(object_index)) )
         throw std::invalid_argument("Object index is not valid.");
   
      uint32_t original_types_size   = types_full.size();
      if( types_minimal.size() != original_types_size )
         throw std::runtime_error("Invariant failure: types_full and types_minimal vectors are out of sync.");

      uint32_t original_minimal_members_size = members_minimal.size();
      uint32_t original_full_members_size    = members_full.size();
      try
      {
         uint32_t temp = (indices.size() << 24) | object_index;
         types_minimal.push_back( temp );
         types_full.push_back( temp );

         for( const auto& ti : indices )
         {
            uint32_t minimal_members_offset = 0;
            uint32_t full_members_offset = 0;
            uint32_t storage = 0;
            unique_window::set(storage, ti.unique);
            ascending_window::set(storage, ti.ascending);
            auto tc = ti.key_type.get_type_class();
            if( tc == type_id::builtin_type )
            {
               if( ti.mapping.size() != 1 )
                 throw std::invalid_argument("Mapping must contain a single entry if mapping object member to a builtin type.");
               index_type_window::set(storage, false); // Specify that the key is a builtin. Redundant since the bit would by default be 0 anyway.
               index_window::set(storage, static_cast<uint32_t>(ti.key_type.get_builtin_type()));
               std::tie(minimal_members_offset, full_members_offset) = add_struct_view(object_index, ti.key_type.get_builtin_type(), ti.mapping.front());
            }
            else if( tc == type_id::struct_type )
            {
               index_type_window::set(storage, true); // Specify that the key is a struct.
               index_window::set(storage, ti.key_type.get_type_index());
               std::tie(minimal_members_offset, full_members_offset) = add_struct_view(object_index, ti.key_type.get_type_index(), ti.mapping);
            }
            else
               throw std::invalid_argument("Key types can only be structs or builtins.");

            types_minimal.push_back(storage);
            types_minimal.push_back(minimal_members_offset);
            
            types_full.push_back(storage);
            types_full.push_back(full_members_offset);
         }
      }
      catch( ... )
      {
         types_minimal.resize(original_types_size);
         types_full.resize(original_types_size);
         members_minimal.resize(original_minimal_members_size);
         members_full.resize(original_full_members_size);
         throw;
      }

      table_lookup.emplace(object_index, original_types_size);
      valid_table_start_indices.insert(original_types_size);
      return original_types_size;
   }

   pair<uint32_t, uint32_t> types_constructor::add_struct_view(type_id::index_t object_index, type_id::builtin builtin_type, uint16_t object_member_index)
   {
      type_id key_type(builtin_type);

      struct_view_entry sve;
       

      auto itr = types_full.begin() + object_index + 1;
      auto object_members_offset = *itr;
      ++itr;
      uint16_t object_num_sorted_members = (*itr & 0xFFFF);
      uint16_t object_total_num_members  = (*itr >> 16);

      if( object_member_index >= object_total_num_members )
         throw std::invalid_argument("The object member index is not valid.");
 
      auto member_itr = members_full.begin() + object_members_offset + object_num_sorted_members + object_member_index;
      if( member_itr->get_type_id() != key_type )
        throw std::invalid_argument("Key type does not match the type of the selected member of the object.");
      
      if( (members_full.size() + 1) > std::numeric_limits<uint32_t>::max() )
         throw std::logic_error("Members vector would be too large.");

      auto m = *member_itr;
      m.set_sort_order(field_metadata::ascending);
      
      auto index_minimal = members_minimal.size();
      auto index_full    = members_full.size();

      members_minimal.push_back(m);
      members_full.push_back(m);

      return {index_minimal, index_full};
   }

   pair<uint32_t, uint32_t> types_constructor::add_struct_view(type_id::index_t object_index, type_id::index_t key_index, const vector<uint16_t>& mapping)
   {
      if( !is_type_valid(type_id::make_struct(key_index)) )
         throw std::invalid_argument("Key index is not valid.");

      if( key_index == object_index )
         throw std::invalid_argument("Key and object cannot be the same struct.");

      auto itr = types_full.begin() + object_index + 1;
      auto object_members_offset = *itr;
      ++itr;
      uint32_t object_num_sorted_members = (*itr & 0xFFFF);
      uint32_t object_total_num_members  = (*itr >> 16);

      itr = types_full.begin() + key_index + 1;
      auto key_members_offset = *itr;
      ++itr;
      uint32_t key_num_sorted_members = (*itr & 0xFFFF);

      if( mapping.size() != key_num_sorted_members )
         throw std::invalid_argument("Mapping size is not equal to the number of sorted members in the specified key type.");

      if( (members_full.size() + key_num_sorted_members) > std::numeric_limits<uint32_t>::max() )
         throw std::logic_error("Members vector would be too large.");

      vector<field_metadata> member_metadata;
      member_metadata.reserve(key_num_sorted_members);

      for( uint32_t i = 0; i < key_num_sorted_members; ++i )
      {
         uint32_t object_member_index = mapping[i];

         if( object_member_index >= object_total_num_members )
           throw std::invalid_argument("An object member index in mapping is invalid.");

         auto object_member_itr = members_full.begin() + object_members_offset + object_num_sorted_members + object_member_index;
         auto key_member_itr    = members_full.begin() + key_members_offset + i;

         if( object_member_itr->get_type_id() != key_member_itr->get_type_id() )
            throw std::invalid_argument("Mapping specified between different types.");

         auto m = *object_member_itr;
         m.set_sort_order( key_member_itr->get_sort_order() );
         member_metadata.push_back(m);
      }

      auto index_minimal = members_minimal.size();
      auto index_full    = members_full.size();

      members_minimal.reserve(index_minimal + key_num_sorted_members);
      std::copy(member_metadata.begin(), member_metadata.end(), std::back_inserter(members_minimal));

      members_full.reserve(index_full + key_num_sorted_members);
      std::copy(member_metadata.begin(), member_metadata.end(), std::back_inserter(members_full));

      return {index_minimal, index_full};
   }

   bool types_constructor::is_type_valid(type_id tid)const
   {
      check_disabled();

      if( tid.is_void() )
         return false;

      auto tc = tid.get_type_class();
      switch( tc )
      {
         case type_id::small_array_of_builtins_type:
         case type_id::builtin_type:
            return true;
         case type_id::small_array_type:
         case type_id::vector_of_something_type:
            return is_type_valid(tid.get_element_type());
         default:
            break;
      }

      auto index = tid.get_type_index();
      if( index >= types_full.size() )
         return false;

      switch( tc )
      {
         case type_id::optional_struct_type:
         case type_id::struct_type:
            if( index == active_struct_index )
               return true;
            else if( valid_struct_start_indices.find(index) != valid_struct_start_indices.end() )
               return true;
            return (valid_tuple_start_indices.find(index) != valid_tuple_start_indices.end());
         case type_id::array_type:
            return (valid_array_start_indices.find(index) != valid_array_start_indices.end());
         case type_id::vector_type:
            return (valid_vector_start_indices.find(index) != valid_vector_start_indices.end());
         case type_id::variant_or_optional_type:
            return (valid_sum_type_start_indices.find(index) != valid_sum_type_start_indices.end());
         default:
            break;
      }
      
      return false; // Should never actually reach here. Just here to make compiler quiet.
   }

   bool types_constructor::is_complete()const
   {
      check_disabled();
   
      if( types_full.size() == 0 || members_full.size() == 0 )
         return false;

      if( incomplete_structs.size() > 0 )
         return false;
      
      return true; 
   }

   void types_constructor::complete_size_align_cache()
   {
      check_disabled();

      if( !is_complete() )
         throw std::logic_error("All declared structs must first be defined before complete_size_align_cache can be called.");

      if( num_types_with_cacheable_size_align != num_types_with_cached_size_align_minimal )
      {
         for( const auto& p : valid_struct_start_indices ) // Caching size_align of structs and tuples must come first.
            types_manager_common(types_minimal, members_minimal).get_size_align(type_id::make_struct(p.first), &num_types_with_cached_size_align_minimal);
         for( auto index : valid_tuple_start_indices )
            types_manager_common(types_minimal, members_minimal).get_size_align(type_id::make_struct(index), &num_types_with_cached_size_align_minimal);

         for( auto index : valid_array_start_indices)
            types_manager_common(types_minimal, members_minimal).get_size_align(type_id::make_array(index), &num_types_with_cached_size_align_minimal);

         for( auto index : valid_sum_type_start_indices)
            types_manager_common(types_minimal, members_minimal).get_size_align(type_id::make_variant(index), &num_types_with_cached_size_align_minimal);
            // Despite the name, the above line also handles optionals.

         if( num_types_with_cacheable_size_align != num_types_with_cached_size_align_minimal )
            throw std::runtime_error("Something went wrong when completing the size_align cache.");
      }

      if( num_types_with_cacheable_size_align != num_types_with_cached_size_align_full )
      {
         for( const auto& p : valid_struct_start_indices ) // Caching size_align of structs and tuples must come first.
            types_manager_common(types_full, members_full).get_size_align(type_id::make_struct(p.first), &num_types_with_cached_size_align_full);
         for( auto index : valid_tuple_start_indices )
            types_manager_common(types_full, members_full).get_size_align(type_id::make_struct(index), &num_types_with_cached_size_align_full);

         for( auto index : valid_array_start_indices)
            types_manager_common(types_full, members_full).get_size_align(type_id::make_array(index), &num_types_with_cached_size_align_full);

         for( auto index : valid_sum_type_start_indices)
            types_manager_common(types_full, members_full).get_size_align(type_id::make_variant(index), &num_types_with_cached_size_align_full);
            // Despite the name, the above line also handles optionals.

         if( num_types_with_cacheable_size_align != num_types_with_cached_size_align_full )
            throw std::runtime_error("Something went wrong when completing the size_align cache.");
      }
   }

   template<class S>
   inline
   typename std::enable_if<std::is_same<S, set<type_id::index_t>>::value, type_id::index_t>::type
   get_index_from_key(const S& s, typename S::const_iterator itr)
   {
      if( itr == s.cend() )
         return type_id::type_index_limit;
      return *itr;
   }

   template<class V>
   inline
   typename std::enable_if<std::is_same<V, vector<pair<type_id::index_t, uint32_t>>>::value, type_id::index_t>::type
   get_index_from_key(const V& v, typename V::const_iterator itr)
   {
      if( itr == v.cend() )
         return type_id::type_index_limit;
      return itr->first;
   }

   pair<types_manager, full_types_manager> types_constructor::destructively_extract_types_managers()
   {
      if( disabled )
         throw std::logic_error("types_manager and full_types_manager have already been extracted from this types_constructor.");

      if( types_full.size() == 0 )
         throw std::logic_error("No types have been defined.");

      if( !is_complete() )
         throw std::logic_error("Some declared structs have not yet been defined.");

      if( num_types_with_cacheable_size_align != num_types_with_cached_size_align_minimal
          || num_types_with_cacheable_size_align != num_types_with_cached_size_align_full )
         complete_size_align_cache();

      flat_map<string, type_id::index_t>  table_lookup_by_name;
      table_lookup_by_name.reserve(table_lookup.size());
      for( const auto& p : struct_lookup )
      {
         auto itr = table_lookup.find(p.second);
         if( itr == table_lookup.end() )
            continue;

         table_lookup_by_name.emplace_hint(table_lookup_by_name.cend(), p.first, itr->second); 
         // Insertion time complexity should be constant since struct_lookup is sorted in same order as table_lookup_by_name.
      } 

      vector<string> all_field_names;
      all_field_names.reserve(field_names.size());
      
      auto field_names_itr = field_names.begin();
      for( uint64_t i = 0; field_names_itr != field_names.end(); ++field_names_itr, ++i )
      {
         field_names_itr->second = i;
         all_field_names.push_back(field_names_itr->first);
      }

      vector<pair<type_id::index_t, uint32_t>> struct_index_map;
      struct_index_map.reserve(struct_lookup.size());

      flat_map<string, uint32_t> lookup_by_name;      
      lookup_by_name.reserve( struct_lookup.size() );
      uint32_t counter = 0;
      for(const auto& p : struct_lookup )
      {
         lookup_by_name.emplace_hint(lookup_by_name.cend(), p.first, 0);
         // Insertion time complexity should be constant since struct_lookup is sorted in same order as lookup_by_name.
        
         struct_index_map.emplace_back(p.second, counter); 
         ++counter;
      }

      auto compare_on_first = [](const typename decltype(struct_index_map)::value_type& lhs, const typename decltype(struct_index_map)::value_type& rhs)
      {
         return lhs.first < rhs.first;
      };
      std::sort(struct_index_map.begin(), struct_index_map.end(), compare_on_first);
               
      vector<uint32_t> valid_indices;
      uint32_t expected_count =  valid_struct_start_indices.size() + valid_tuple_start_indices.size() + valid_table_start_indices.size()
                                  + valid_array_start_indices.size() + valid_vector_start_indices.size() + valid_sum_type_start_indices.size();
      uint32_t actual_count   = 0;
      valid_indices.reserve(expected_count + valid_table_start_indices.size() + 3 * valid_struct_start_indices.size());

      auto table_itr    = valid_table_start_indices.begin(); 
      auto struct_itr   = struct_index_map.begin(); // The keys of struct_index_map should be the same thing as the sorted keys of valid_struct_start_indices
      auto struct_itr2  = struct_fields_map.begin(); // Should have identical keys as struct_index_map. 
      auto tuple_itr    = valid_tuple_start_indices.begin(); 
      auto array_itr    = valid_array_start_indices.begin(); 
      auto vector_itr   = valid_vector_start_indices.begin(); 
      auto sum_type_itr = valid_sum_type_start_indices.begin(); 

      vector<type_id::index_t> next_indices;
      next_indices.resize(6);
      next_indices[0] = get_index_from_key(valid_table_start_indices,    table_itr);
      next_indices[1] = get_index_from_key(struct_index_map,             struct_itr);
      next_indices[2] = get_index_from_key(valid_tuple_start_indices,    tuple_itr);
      next_indices[3] = get_index_from_key(valid_array_start_indices,    array_itr);
      next_indices[4] = get_index_from_key(valid_vector_start_indices,   vector_itr);
      next_indices[5] = get_index_from_key(valid_sum_type_start_indices, sum_type_itr);

      using index_type = full_types_manager::index_type;

      vector<uint64_t> fields_info;
      
      types_manager_common tm(types_full, members_full);
      uint32_t cont_storage = 0;
      full_types_manager::continuation_window::set(cont_storage, true);
      while(true)
      {
         auto itr = std::min_element(next_indices.begin(), next_indices.end());
         if( *itr == type_id::type_index_limit )
            break;

         ++actual_count;
         auto index = *itr;

         uint32_t storage = 0;
         full_types_manager::index_window::set(storage, index);
         switch( itr - next_indices.begin() )
         {
            case 0: // table
            {
               // types_constructor processes all structs in ABI prior to processing any table, so it is safe to assume
               // that the struct for this table object has already been processed in a previous iteration of this while loop.
               
               auto num_index = tm.get_num_indices_in_table(index);
               auto struct_index = tm.get_struct_index_of_table_object(index);

               auto itr2 = std::lower_bound(struct_index_map.begin(), struct_index_map.end(), std::make_pair(struct_index, 0), compare_on_first);
               if( itr2 == struct_index_map.end() )
                  throw std::runtime_error("Invariant failure: Could not find struct for the table object.");
               auto itr3 = lookup_by_name.begin() + itr2->second;
               auto object_entry_index = itr3->second; // At this point in the while loop iteration, itr3->second holds the index into valid_indices for the entry of the table object struct.
               itr3->second = valid_indices.size();    // But now we replace it with the index to the entry for the table itself.
              
               full_types_manager::index_type_window::set(storage, static_cast<uint8_t>(index_type::table_index));
               if( (num_index & 0x80) > 0) 
               full_types_manager::three_bits_window::set(storage, (num_index & 0x60) ); // Store high 3 bits of num_indices in first part of the entry.
               valid_indices.push_back(storage);

               storage = cont_storage;
               full_types_manager::five_bits_window::set(storage, (num_index & 0x1F) ); // Store low 5 bits of num_indices in the second part of the entry.
               full_types_manager::large_index_window::set(storage, object_entry_index); // Store index into valid_indices of the table object struct.
               valid_indices.push_back(storage);
               
               ++table_itr;
               next_indices[0] = get_index_from_key(valid_table_start_indices,    table_itr);
               break;
            }
            case 1: // struct
            {
               uint64_t fields_info_index = fields_info.size();
 
               if( struct_itr2->first != index )
                  throw std::runtime_error("Invariant failure: keys of struct_fields_map do not match keys of struct_index_map."); 

               auto struct_fields_index = full_types_manager::fields_index_window::get(struct_itr2->second);

               uint16_t total_num_members;
               std::tie(std::ignore, std::ignore, total_num_members) = tm.get_members_common(index);

               type_id::index_t base_index;
               std::tie(base_index, std::ignore) = tm.get_base_info(index); 
               if( base_index == type_id::type_index_limit )
               {
                 full_types_manager::index_type_window::set(storage, static_cast<uint8_t>(index_type::simple_struct_index));
               }
               else
               {
                 full_types_manager::index_type_window::set(storage, static_cast<uint8_t>(index_type::derived_struct_index));
                 --total_num_members; // Total number of members is now total number of fields. 
               }

               auto r = make_range(struct_fields, struct_fields_index, struct_fields_index + total_num_members);
               for( const auto& p : r )
               {
                  uint64_t storage2 = 0;
                  full_types_manager::fields_index_window::set(storage2, p.first->second);
                  full_types_manager::sort_order_window::set(storage2, p.second);
                  fields_info.push_back(storage2);
               } 

               (lookup_by_name.begin() + struct_itr->second)->second = valid_indices.size();

               // Store high 3 bits of fields_info_index (bits 37 to 39) in first part of the entry.
               full_types_manager::three_bits_window::set(storage, static_cast<uint8_t>(fields_info_index >> 37));
               valid_indices.push_back(storage);

               storage = cont_storage;
               full_types_manager::index_window::set(storage, struct_itr->second); // Index into lookup_by_name
               // Store next 7 bits of fields_info_index (bits 30 to 36) in second part of the entry.
               full_types_manager::seven_bits_window::set(storage, static_cast<uint8_t>((fields_info_index & 0x1FC0000000) >> 30));
               valid_indices.push_back(storage);

               storage = cont_storage;
               full_types_manager::field_size_window::set(storage, total_num_members);
               // Store next 15 bits of fields_info_index (bits 15 to 29) in third part of the entry.
               full_types_manager::fifteen_bits_window::set(storage, static_cast<uint16_t>((fields_info_index & 0x3FFF8000) >> 15));
               valid_indices.push_back(storage);

               storage = cont_storage;
               full_types_manager::base_sort_window::set(storage, full_types_manager::sort_order_window::get(struct_itr2->second));
               // Store low 15 bits of fields_info_index (bits 0 to 14) in third part of the entry.
               full_types_manager::fifteen_bits_window::set(storage, static_cast<uint16_t>(fields_info_index & 0x7FFF));
               valid_indices.push_back(storage);

               ++struct_itr;
               ++struct_itr2;
               next_indices[1] = get_index_from_key(struct_index_map,             struct_itr);
               break;
            }
            case 2: // tuple
               full_types_manager::index_type_window::set(storage, static_cast<uint8_t>(index_type::tuple_index)); 
               valid_indices.push_back(storage);
               ++tuple_itr;
               next_indices[2] = get_index_from_key(valid_tuple_start_indices,    tuple_itr);
               break;
            case 3: // array
               full_types_manager::index_type_window::set(storage, static_cast<uint8_t>(index_type::array_index)); 
               valid_indices.push_back(storage);
               ++array_itr;
               next_indices[3] = get_index_from_key(valid_array_start_indices,    array_itr);
               break;
            case 4: // vector
               full_types_manager::index_type_window::set(storage, static_cast<uint8_t>(index_type::vector_index)); 
               valid_indices.push_back(storage);
               ++vector_itr;
               next_indices[4] = get_index_from_key(valid_vector_start_indices,   vector_itr);
               break;
            case 5: // sum_type
               full_types_manager::index_type_window::set(storage, static_cast<uint8_t>(index_type::sum_type_index)); 
               valid_indices.push_back(storage);
               ++sum_type_itr;
               next_indices[5] = get_index_from_key(valid_sum_type_start_indices, sum_type_itr);
               break;
            default:
               throw std::runtime_error("Unexpected result: this case should have never been reached.");
         }
      }
      if( actual_count != expected_count )
         throw std::runtime_error("Unexpected result: number of entries in merged indices map is not correct.");

      disabled = true;
      types_manager mtm(std::move(types_minimal), std::move(members_minimal), std::move(table_lookup_by_name));
      full_types_manager ftm(std::move(types_full), std::move(members_full), std::move(lookup_by_name), 
                             std::move(valid_indices), std::move(all_field_names), std::move(fields_info));

      return {std::move(mtm), std::move(ftm)};
   }

   type_id::size_align
   types_constructor::get_size_align(type_id tid)const
   {
      return types_manager_common(types_minimal, members_minimal).get_size_align(tid, &num_types_with_cached_size_align_minimal);
   }


} }

