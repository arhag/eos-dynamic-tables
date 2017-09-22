#include <eos/types/types_constructor.hpp>
#include <eos/types/bit_view.hpp>

#include <ostream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <tuple>
#include <limits>
#include <cctype>
#include <boost/io/ios_state.hpp>
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
            auto& field_names = itr->second;

            type_id base;
            if( t.second != -1 )
            {
               auto base_tid = type_id::make_struct(static_cast<type_id::index_t>(t.second));
               base = remap_abi_type(abi, index_map, struct_map, base_tid);
            }

            vector<pair<type_id, int16_t>> fields;
            for( const auto& p : s.fields )
            {
               if( !is_name_valid(p.first) )
                  throw std::invalid_argument("Name of field is not valid.");

               if( !field_names.insert(p.first).second )
                  throw std::invalid_argument("Another field with same name already exists within the struct.");

               auto t = remap_abi_type(abi, index_map, struct_map, p.second);
               fields.emplace_back(t, 0);
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
                  fields.at(j-1).second = counter;                  
               else
                  fields.at(-(j+2)).second = -counter;
               ++counter;
            }

            return add_struct(s.name, fields, base, base_sort);
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

      tm.table_lookup.reserve(abi.tables.size());
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

         auto index = add_table(object_index, indices);
         tm.table_lookup.emplace( abi.structs[abi.types[tbl.object_index].first].name, index );
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

         auto index = tm.add_empty_struct_to_end();
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

      auto index = tm.add_empty_struct_to_end();
      incomplete_structs.emplace(declared_name, index);
      valid_struct_start_indices.emplace(index, declared_name); 
      ++num_types_with_cacheable_size_align;

      return index;
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

      if( !was_previously_declared && (tm.types.size() >= type_id::type_index_limit) )
         throw std::logic_error("Not enough room to add a struct with valid index."); 

      if( (tm.members.size() + 2 + 2 * fields.size()) > std::numeric_limits<uint32_t>::max() )
         throw std::logic_error("Fields vector could become too large.");

      type_id::index_t index = ( was_previously_declared ? incomplete_itr->second : tm.add_empty_struct_to_end() );
      
      active_struct_index = index;
      uint32_t original_members_size = tm.members.size();
      try 
      {  
         type_id::size_align sa;
         uint16_t num_sorted_members = 0;
         std::tie(sa, num_sorted_members) = process_struct(fields, base, base_sort); // Actually adds field_metadata to end of members
         // If anything (meaning the process_struct function) throws an exception up to this point from the start of the try block,
         // then this function will be able to recover the state prior to being called.
         
         field_metadata::sort_order base_so = ( (base_sort == 0) ? field_metadata::unsorted 
                                                                 : ( (base_sort > 0) ? field_metadata::ascending
                                                                                     : field_metadata::descending ) );

         tm.complete_struct(index, sa, 
                            original_members_size, (base.is_void() ? fields.size() : fields.size() + 1), num_sorted_members,
                            base, base_so);
      } 
      catch( ... ) 
      {
         active_struct_index = type_id::type_index_limit;
         if( !was_previously_declared )
            tm.types.resize(index);
         tm.members.resize(original_members_size);
         throw;
      }
      active_struct_index = type_id::type_index_limit;

      struct_lookup.emplace(name, index);
      valid_struct_start_indices.emplace(index, name); 
      if( was_previously_declared )
         incomplete_structs.erase(incomplete_itr);
      else
         ++num_types_with_cacheable_size_align;
      ++num_types_with_cached_size_align;
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

         tm.members.push_back(member_metadata[p.second]);
         //sort_order.push_back(p.second);
      }

      for( auto f : member_metadata )
         tm.members.push_back(f);

      return {type_id::size_align(size, align), fobso_map.size()};
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

      if( tm.types.size() >= type_id::type_index_limit)
         throw std::logic_error("Not enough room to add a tuple with valid index.");

      if( (tm.members.size() + 2 * field_types.size()) > std::numeric_limits<uint32_t>::max() )
         throw std::logic_error("Fields vector would become too large.");

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

      type_id::index_t index = tm.add_empty_struct_to_end();
      
      active_struct_index = index;
      uint32_t original_members_size = tm.members.size();
      try 
      {  
         type_id::size_align sa;
         uint16_t num_sorted_members = 0;
         std::tie(sa, num_sorted_members) = process_struct(fields, type_id(), 0); // Actually adds field_metadata to end of members
         // If anything (meaning the process_struct function) throws an exception up to this point from the start of the try block,
         // then this function will be able to recover the state prior to being called.

         tm.complete_struct(index, sa, 
                            original_members_size, fields.size(), num_sorted_members,
                            type_id(), field_metadata::unsorted);
      } 
      catch( ... ) 
      {
         active_struct_index = type_id::type_index_limit;
         tm.types.resize(index);
         tm.members.resize(original_members_size);
         throw;
      }
      active_struct_index = type_id::type_index_limit;

      tuple_lookup.emplace(field_types, index);
      valid_tuple_start_indices.insert(index);
      ++num_types_with_cacheable_size_align;
      ++num_types_with_cached_size_align;
      return index;
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

      if( tm.types.size() >= type_id::type_index_limit)
         throw std::logic_error("Not enough room to add an array with valid index.");

      if( !is_type_valid(element_type) )
         throw std::invalid_argument("Invalid element type for adding an array.");

      type_id::index_t index = tm.add_array(element_type, num_elements);

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

      if( tm.types.size() >= type_id::type_index_limit)
         throw std::logic_error("Not enough room to add a vector with valid index.");

      if( !is_type_valid(element_type) )
         throw std::invalid_argument("Invalid element type for adding a vector.");

      type_id::index_t index = tm.add_vector(element_type);

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

      if( tm.types.size() >= type_id::type_index_limit)
         throw std::logic_error("Not enough room to add an optional with valid index.");

      if( !is_type_valid(element_type) )
         throw std::invalid_argument("Invalid element type for adding an optional.");

      type_id::index_t index = tm.add_optional(element_type);

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

      if( tm.types.size() >= type_id::type_index_limit)
         throw std::logic_error("Not enough room to add a variant with valid index.");

      for( auto t : cases )
         if( !is_type_valid(t) )
            throw std::invalid_argument("Invalid type within variant that was to be added.");

      type_id::index_t index = tm.add_variant(cases);
      
      variant_lookup.emplace(cases, index);
      valid_sum_type_start_indices.insert(index);
      ++num_types_with_cacheable_size_align;
      return index;
   }

   type_id::index_t types_constructor::add_table(type_id::index_t object_index, const vector<ABI::table_index>& indices)
   {
      check_disabled();
      
      // TODO; Better solution required
      using unique_window     = types_manager::unique_window;
      using ascending_window  = types_manager::ascending_window;
      using index_type_window = types_manager::index_type_window;
      using index_window      = types_manager::index_window;

      auto itr = table_lookup.find(object_index);
      if( itr != table_lookup.end() )
         throw std::logic_error("Table already exists for the specified object.");   

      if( indices.size() >= 256 )
         throw std::invalid_argument("Tables cannot support that many indices.");

      if( !is_type_valid(type_id::make_struct(object_index)) )
         throw std::invalid_argument("Object index is not valid.");

      uint32_t original_types_size   = tm.types.size();
      uint32_t original_members_size = tm.members.size();
      try
      {
         tm.types.push_back( static_cast<uint32_t>(indices.size() << 24) | object_index );
         for( const auto& ti : indices )
         {
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
               tm.types.push_back(storage);
               tm.types.push_back(add_struct_view(object_index, ti.key_type.get_builtin_type(), ti.mapping.front()));
            }
            else if( tc == type_id::struct_type )
            {
               index_type_window::set(storage, true); // Specify that the key is a struct.
               index_window::set(storage, ti.key_type.get_type_index());
               tm.types.push_back(storage);
               tm.types.push_back(add_struct_view(object_index, ti.key_type.get_type_index(), ti.mapping));
            }
            else
               throw std::invalid_argument("Key types can only be structs or builtins.");
         }
      }
      catch( ... )
      {
         tm.types.resize(original_types_size);
         tm.members.resize(original_members_size);
         throw;
      }

      table_lookup.emplace(object_index, original_types_size);
      valid_table_start_indices.insert(original_types_size);
      return original_types_size;
   }

   uint32_t types_constructor::add_struct_view(type_id::index_t object_index, type_id::builtin builtin_type, uint16_t object_member_index)
   {
      /*
      check_disabled();

      if( !is_type_valid(type_id::make_struct(object_index)) )
         throw std::invalid_argument("Object index is not valid.");
      */

      type_id key_type(builtin_type);

      struct_view_entry sve;
       

      auto itr = tm.types.begin() + object_index + 1;
      auto object_members_offset = *itr;
      ++itr;
      uint16_t object_num_sorted_members = (*itr & 0xFFFF);
      uint16_t object_total_num_members  = (*itr >> 16);

      if( object_member_index >= object_total_num_members )
         throw std::invalid_argument("The object member index is not valid.");
 
      auto member_itr = tm.members.begin() + object_members_offset + object_num_sorted_members + object_member_index;
      if( member_itr->get_type_id() != key_type )
        throw std::invalid_argument("Key type does not match the type of the selected member of the object.");
      
      if( (tm.members.size() + 1) > std::numeric_limits<uint32_t>::max() )
         throw std::logic_error("Fields vector would be too large.");

      auto index = tm.members.size();
      auto m = *member_itr;
      m.set_sort_order(field_metadata::ascending);
      tm.members.push_back(m);

      return index;
   }

   uint32_t types_constructor::add_struct_view(type_id::index_t object_index, type_id::index_t key_index, const vector<uint16_t>& mapping)
   {
      /*
      check_disabled();

      if( !is_type_valid(type_id::make_struct(object_index)) )
         throw std::invalid_argument("Object index is not valid.");
      */

      if( !is_type_valid(type_id::make_struct(key_index)) )
         throw std::invalid_argument("Key index is not valid.");

      if( key_index == object_index )
         throw std::invalid_argument("Key and object cannot be the same struct.");

      auto itr = tm.types.begin() + object_index + 1;
      auto object_members_offset = *itr;
      ++itr;
      uint32_t object_num_sorted_members = (*itr & 0xFFFF);
      uint32_t object_total_num_members  = (*itr >> 16);

      itr = tm.types.begin() + key_index + 1;
      auto key_members_offset = *itr;
      ++itr;
      uint32_t key_num_sorted_members = (*itr & 0xFFFF);

      if( mapping.size() != key_num_sorted_members )
         throw std::invalid_argument("Mapping size is not equal to the number of sorted members in the specified key type.");

      if( (tm.members.size() + key_num_sorted_members) > std::numeric_limits<uint32_t>::max() )
         throw std::logic_error("Fields vector would be too large.");

      vector<field_metadata> member_metadata;
      member_metadata.reserve(key_num_sorted_members);

      for( uint32_t i = 0; i < key_num_sorted_members; ++i )
      {
         uint32_t object_member_index = mapping[i];

         if( object_member_index >= object_total_num_members )
           throw std::invalid_argument("An object member index in mapping is invalid.");

         auto object_member_itr = tm.members.begin() + object_members_offset + object_num_sorted_members + object_member_index;
         auto key_member_itr    = tm.members.begin() + key_members_offset + i;

         if( object_member_itr->get_type_id() != key_member_itr->get_type_id() )
            throw std::invalid_argument("Mapping specified between different types.");

         auto m = *object_member_itr;
         m.set_sort_order( key_member_itr->get_sort_order() );
         member_metadata.push_back(m);
      }

      auto index = tm.members.size();
      tm.members.reserve(tm.members.size() + key_num_sorted_members);
      std::copy(member_metadata.begin(), member_metadata.end(), std::back_inserter(tm.members));

      return index;
   }

   string types_constructor::get_struct_name(type_id::index_t index)const
   {
      check_disabled();

      auto itr = valid_struct_start_indices.find(index);
      if( itr == valid_struct_start_indices.end() )
      {
         if( valid_tuple_start_indices.find(index) == valid_tuple_start_indices.end() )
            throw std::invalid_argument("Not a valid struct index.");
         else
            throw std::invalid_argument("Struct index points to a tuple and so it does not have a name.");
      }

      return itr->second;
   }

   type_id::index_t types_constructor::get_struct_index(const string& name)const
   {
      check_disabled();

      auto itr1 = struct_lookup.find(name);
      if( itr1 != struct_lookup.end() )
         return itr1->second;

      auto itr2 = incomplete_structs.find(name);
      if( itr2 != incomplete_structs.end() )
         return itr2->second;
      
      throw std::logic_error("Struct not found.");
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
      if( index >= tm.types.size() )
         return false;

      switch( tid.get_type_class() )
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
   
      if( tm.types.size() == 0 || tm.members.size() == 0 )
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

      if( num_types_with_cacheable_size_align == num_types_with_cached_size_align )
         return;

      for( const auto& p : valid_struct_start_indices ) // Caching size_align of structs and tuples must come first.
         get_size_align(type_id::make_struct(p.first));
      for( auto index : valid_tuple_start_indices )
         get_size_align(type_id::make_struct(index));

      for( auto index : valid_array_start_indices)
         get_size_align(type_id::make_array(index));

      for( auto index : valid_sum_type_start_indices)
         get_size_align(type_id::make_variant(index)); // Despite the name, it also works for optionals.

      if( num_types_with_cacheable_size_align != num_types_with_cached_size_align )
         throw std::runtime_error("Something went wrong when completing the size_align cache.");
   }

   types_manager types_constructor::copy_types_manager()
   {
      if( disabled )
         throw std::logic_error("types_manager has already been extracted from this types_constructor.");

      if( tm.types.size() == 0 )
         throw std::logic_error("No types have been defined.");

      if( !is_complete() )
         throw std::logic_error("Some declared structs have not yet been defined.");

      if( num_types_with_cacheable_size_align != num_types_with_cached_size_align )
         complete_size_align_cache();

      return tm;
   }

   types_manager types_constructor::destructively_extract_types_manager()
   {
      if( disabled )
         throw std::logic_error("types_manager has already been extracted from this types_constructor.");

      if( tm.types.size() == 0 )
         throw std::logic_error("No types have been defined.");

      if( !is_complete() )
         throw std::logic_error("Some declared structs have not yet been defined.");

      if( num_types_with_cacheable_size_align != num_types_with_cached_size_align )
         complete_size_align_cache();

      disabled = true;
      return {std::move(tm.types), std::move(tm.members), std::move(tm.table_lookup)};
   }

   type_id::size_align
   types_constructor::get_size_align(type_id tid)const
   {
      return tm.get_size_align(tid, &num_types_with_cached_size_align);
   }

   void types_constructor::check_struct_index(type_id::index_t struct_index)const
   {
      if( (valid_struct_start_indices.find(struct_index) == valid_struct_start_indices.end()) 
          && (valid_tuple_start_indices.find(struct_index) == valid_tuple_start_indices.end()) )
         throw std::invalid_argument("Struct does not exist.");
   }

   type_id::size_align 
   types_constructor::get_size_align_of_struct(type_id::index_t struct_index)const
   {
      check_disabled();
      check_struct_index(struct_index);

      return get_size_align(type_id::make_struct(struct_index));
   }

   range<typename vector<field_metadata>::const_iterator> 
   types_constructor::get_all_members(type_id::index_t struct_index)const
   {
      check_disabled();
      check_struct_index(struct_index);

      return tm.get_all_members(struct_index);
   }

   range<typename vector<field_metadata>::const_iterator> 
   types_constructor::get_sorted_members(type_id::index_t struct_index)const
   {
      check_disabled();
      check_struct_index(struct_index);

      return tm.get_sorted_members(struct_index);
   }

   struct print_type_visitor
   {
      const types_constructor& tc;
      std::ostream&      os;
      vector<type_id::index_t>& struct_indices;
      map<type_id::index_t, uint32_t>& struct_index_map;
      uint32_t template_start_index;
      bool start_of_variant;

      print_type_visitor(const types_constructor& tc, std::ostream& os, vector<type_id::index_t>& struct_indices, map<type_id::index_t, uint32_t>& struct_index_map, uint32_t template_start_index) 
         : tc(tc), os(os), struct_indices(struct_indices), struct_index_map(struct_index_map), template_start_index(template_start_index), start_of_variant(false)
      {}

      using traversal_shortcut = types_manager::traversal_shortcut;
      using array_type    = types_manager::array_type;
      using struct_type   = types_manager::struct_type;
      using vector_type   = types_manager::vector_type;
      using optional_type = types_manager::optional_type;
      using variant_type  = types_manager::variant_type; 

      template<typename T>
      traversal_shortcut operator()(const T&) { return types_manager::no_shortcut; } // Default implementation
      template<typename T, typename U>
      traversal_shortcut operator()(const T&, U) { return types_manager::no_shortcut; } // Default implementation

      traversal_shortcut operator()(type_id::builtin b)
      {
         os << type_id::get_builtin_type_name(b);
         return types_manager::no_shortcut; 
      }

      traversal_shortcut operator()(struct_type t)
      {
         if( tc.valid_tuple_start_indices.find(t.index) != tc.valid_tuple_start_indices.end() )
         {
            auto r = tc.tm.get_all_members(t.index);
            if( r.end() - r.begin() == 2 )
               os << "Pair<";
            else
               os << "Tuple<";

            bool first = true;
            for( auto itr = r.begin(); itr != r.end(); ++itr )
            {
               if( first )
                  first = false;
               else
                  os << ", ";
               tc.tm.traverse_type(itr->get_type_id(), *this);
            }

            os << ">";
            return types_manager::no_deeper;
         }

         auto res = struct_index_map.emplace(t.index, struct_indices.size());
         if( res.second )
            struct_indices.push_back(t.index);
         os << "T" << (res.first->second + template_start_index);
         return types_manager::no_deeper;
      }
      
      traversal_shortcut operator()(variant_type t) 
      {
         os << "Variant<";
         start_of_variant = true;
         return types_manager::no_shortcut;
      }

      traversal_shortcut operator()(variant_type t, uint32_t case_index) 
      {
         if( start_of_variant )
            start_of_variant = false;
         else
            os << ", ";

         return types_manager::no_shortcut;
      }

      traversal_shortcut operator()(variant_type t, bool) 
      {
         os << ">";
         return types_manager::no_shortcut;
      }

      traversal_shortcut operator()(array_type t) 
      { 
         if( t.element_type.get_type_class() == type_id::builtin_type && t.element_type.get_builtin_type() == type_id::builtin_bool)
         {
            os << "Bitset<" << t.num_elements;
         }
         else
         {
            os << "Array<";
            tc.print_type(os, t.element_type, struct_indices, struct_index_map, template_start_index);
            os << ", " << t.num_elements;
         }
         os << ">";
       
         return types_manager::no_deeper; 
      }

      traversal_shortcut operator()(vector_type t) 
      {
         os << "Vector<";
         tc.print_type(os, t.element_type, struct_indices, struct_index_map, template_start_index);
         os << ">";
         return types_manager::no_deeper; 
      }
      
      traversal_shortcut operator()(optional_type t) 
      {
         os << "Optional<";
         tc.print_type(os, t.element_type, struct_indices, struct_index_map, template_start_index);
         os << ">";  
         return types_manager::no_deeper; 
      }
      
      traversal_shortcut operator()() 
      {
         os << "Void";
         return types_manager::no_shortcut;
      }
   };


   void types_constructor::print_type(std::ostream& os, const type_id& tid, vector<uint32_t>& struct_indices, map<uint32_t, uint32_t>& struct_index_map, uint32_t template_start_index)const
   {
      print_type_visitor v(*this, os, struct_indices, struct_index_map, template_start_index);
      tm.traverse_type(tid, v);
   }

   uint32_t types_constructor::print_type(std::ostream& os, const type_id& tid, uint32_t template_start_index, bool inline_struct)const
   {
      check_disabled();

      boost::io::ios_flags_saver ifs( os );

      os << std::dec;
     
      if( !inline_struct && tid.get_type_class() == type_id::struct_type )
      {
         static const char* tab        = "    ";
         static const char* ascending  = "ascending";
         static const char* descending = "descending";

         auto struct_index = tid.get_type_index();
         auto itr = valid_struct_start_indices.find(struct_index);
         if( itr == valid_struct_start_indices.end() )
         {
            if( valid_tuple_start_indices.find(struct_index) == valid_tuple_start_indices.end() )
               throw std::invalid_argument("Struct or tuple does not exist.");

            // Otherwise it is a tuple and so it needs to be handled by the same code that handles non-struct types.
         }
         else
         {
            auto res = tm.get_base_info(itr->first);
            bool base_exists = (res.first != type_id::type_index_limit);
            auto base_itr = valid_struct_start_indices.end();
            if( base_exists )
            {
               base_itr = valid_struct_start_indices.find(res.first);
               if( base_itr == valid_struct_start_indices.end() )
                  throw std::runtime_error("Invariant failure: Base of valid struct does not exist.");
            }

            os << "struct ";
            os <<  itr->second << " /* struct(" << itr->first << ") */";
            if( base_exists )
            {
               os << " : ";
               os << base_itr->second << " /* struct(" << base_itr->first << ") */";
               if( res.second != field_metadata::unsorted )
               {
                  os << " // Base sorted in " << ((res.second == field_metadata::ascending) ? ascending : descending);
                  os << " order";
               }
            }
            os << std::endl << "{" << std::endl;

            auto r = tm.get_all_members(itr->first);
            uint32_t field_seq_num = 0;
            for( auto itr = r.begin() + (base_exists ? 1 : 0); itr != r.end(); ++field_seq_num, ++itr)
            {
               os << tab;
               template_start_index = print_type(os, itr->get_type_id(), template_start_index, true);
               os << " f" << field_seq_num << ";";

               auto so = itr->get_sort_order();
               if( so != field_metadata::unsorted )
                  os << " // Sorted in " << (so == field_metadata::ascending ? ascending : descending) << " order";
               os << std::endl;
            }
            os << "};" << std::endl;

            return template_start_index;
         }
      }

      if( !tid.is_void() && !is_type_valid(tid) )
         throw std::logic_error("Type is not valid.");

      // Else print the non-struct type in typical field format:
      vector<type_id::index_t> struct_indices;
      map<type_id::index_t, uint32_t> struct_index_map;
      print_type(os, tid, struct_indices, struct_index_map, template_start_index);

      if( struct_indices.size() > 0 )
      {
         os << " /*[";
         uint32_t counter = template_start_index;
         for( auto index : struct_indices )
         {
            if( counter > template_start_index )
               os << ", ";

            os << "T" << counter << " = struct(" << index << ")";
            ++counter;
         }
         os << "]*/";
      }

      return struct_indices.size() + template_start_index;
   }

} }

