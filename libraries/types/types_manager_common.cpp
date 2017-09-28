#include <eos/eoslib/types_manager_common.hpp>
#include <eos/eoslib/exceptions.hpp>

namespace eos { namespace types {

   void cache_size_align(uint32_t* num_types_with_cached_size_align_ptr, vector<uint32_t>::const_iterator cache_itr, uint32_t size, uint8_t align)
   {
      if( num_types_with_cached_size_align_ptr == nullptr )
         return;

      const_cast<uint32_t&>(*cache_itr) = type_id::size_align(size, align).get_storage();
      ++(*num_types_with_cached_size_align_ptr);
   }

   type_id::size_align types_manager_common::get_size_align(type_id tid, uint32_t* cache_ptr)const
   {
      uint8_t  align = 4;
      uint32_t size  = 8;
      
      switch( tid.get_type_class() )
      {
         case type_id::builtin_type:
            return type_id::get_builtin_type_size_align(tid.get_builtin_type());
         case type_id::struct_type:
         {
            auto itr = types.begin() + tid.get_type_index();
            type_id::size_align sa(*itr);
            if( !sa.is_complete() )
               return {}; // Incomplete type

            size = sa.get_size();
            align = sa.get_align();
            break;
         }
         case type_id::variant_or_optional_type:
         {
            auto itr = types.begin() + tid.get_type_index();
            type_id::size_align sa(*itr);
            if( sa.is_complete() )
            {
               size = sa.get_size();
               align = sa.get_align();
               break;
            }

            auto cache_itr = itr;
            ++itr;
            if( *itr < type_id::variant_case_limit ) // Variant type
            {
               uint32_t max_size  = 0;
               uint8_t  max_align = 2; // We need two bytes to store all possible variant case indices as a tag.

               auto n = *itr;
               ++itr;
               for( uint32_t i = 0; i < n; ++i, ++itr )
               {
                  type_id t(*itr);
                  auto res = get_size_align(t);
                  if( !res.is_complete() )
                     return {}; // Incomplete type

                  max_size    = std::max(max_size,  res.get_size());
                  max_align   = std::max(max_align, std::min(static_cast<uint8_t>(1), res.get_align())); 
               }

               size  = type_id::round_up_to_alignment(max_size, 2) + 2; // Variant tag is at the end for efficiency of packing given various alignment requirements.
               align = max_align;

               // Cache results
               cache_size_align(cache_ptr, cache_itr, size, align);
            }
            else // Optional type
            {
               auto res = get_size_align(type_id(*itr)); // Get size and align of element type
               if( !res.is_complete() )
                  return {}; // Incomplete type

               align = std::min(static_cast<uint8_t>(1), res.get_align());
               size = res.get_size() + 1; // Add single byte at end to track if type exists.

               // Cache results
               cache_size_align(cache_ptr, cache_itr, size, align);
            }
            break;
         }
         case type_id::optional_struct_type:
         {
            auto res = get_size_align(tid.get_element_type());
            if( !res.is_complete() )
               return {}; // Incomplete type

            align = std::min(static_cast<uint8_t>(1), res.get_align());
            size = res.get_size() + 1; // Add single byte at end to track if type exists.
            break;
         }
         case type_id::vector_type:
         case type_id::vector_of_something_type:
            break;
         case type_id::array_type:
         {
            auto itr = types.begin() + tid.get_type_index();
            type_id::size_align sa(*itr);
            if( sa.is_complete() )
            {
               size = sa.get_size();
               align = sa.get_align();
               break;
            }

            auto cache_itr = itr;
            ++itr;
            auto num_elements = *itr;
            ++itr;

            auto res = get_size_align(type_id(*itr));
            if( !res.is_complete() )
               return {}; // Incomplete type

            align = res.get_align();
            if( align == 0 ) // Must be an array of bools
            {
               align = 1;
               size  = (res.get_size() + 7)/8;
            }
            else
               size = type_id::round_up_to_alignment(res.get_size(), align) * num_elements;

            // Cache results
            cache_size_align(cache_ptr, cache_itr, size, align);
            break;
         }
         case type_id::small_array_of_builtins_type:
         case type_id::small_array_type:
         {
            // No cache to look for. Must always compute size and align.
            auto res = get_size_align(tid.get_element_type());
            if( !res.is_complete() )
               return {}; // Incomplete type

            align = res.get_align();
            if( align == 0 ) // Must be an array of bools
            {
               align = 1;
               size  = (res.get_size() + 7)/8;
            }
            else
               size = type_id::round_up_to_alignment(res.get_size(), align) * tid.get_small_array_size();
            
            break;
         }
      }
      
      return {size, align};
   }

   tuple<uint32_t, uint16_t, uint16_t> 
   types_manager_common::get_members_common(type_id::index_t struct_index)const
   {
      if( struct_index + 3 >= types.size() )
         EOS_ERROR(std::out_of_range, "Struct index is not valid.");

      auto itr = types.begin() + struct_index + 1;
      auto member_data_offset = *itr;
      ++itr;
      uint16_t total_num_members  = (*itr >> 16);
      uint16_t num_sorted_members = (*itr & 0xFFFF);
      
      if( (member_data_offset + num_sorted_members + total_num_members) > members.size() )
         EOS_ERROR(std::runtime_error, "Invariant failure: range would be out of the bounds, most likely because struct_index was not valid.");

      return std::make_tuple(member_data_offset, num_sorted_members, total_num_members);
  }

   range<typename vector<field_metadata>::const_iterator> 
   types_manager_common::get_sorted_members(type_id::index_t struct_index)const
   {
      uint32_t member_data_offset;
      uint16_t num_sorted_members;
      std::tie(member_data_offset, num_sorted_members, std::ignore) = get_members_common(struct_index);
      return make_range(members, member_data_offset, member_data_offset + num_sorted_members); 
   }

   type_id types_manager_common::get_variant_case_type(type_id tid, uint16_t which)const
   { 
      if( tid.get_type_class() != type_id::variant_or_optional_type )
         EOS_ERROR(std::invalid_argument, "Must provide a variant type to this function.");

      auto itr = types.begin() + tid.get_type_index() + 1;

      if( *itr >= type_id::variant_case_limit )
         EOS_ERROR(std::invalid_argument, "Provided type points to an optional, not a variant.");

      if( which >= *itr )
         EOS_ERROR(std::out_of_range, "Case index specified by which is not valid");
      
      itr += 1 + which;

      return type_id(*itr, true);
   }

   uint32_t types_manager_common::get_variant_tag_offset(type_id tid)const
   {
      if( tid.get_type_class() != type_id::variant_or_optional_type )
         EOS_ERROR(std::invalid_argument, "Must provide a variant type to this function.");

      auto itr = types.begin() + tid.get_type_index();
      type_id::size_align sa(*itr);

      ++itr;
      if( *itr >= type_id::variant_case_limit )
         EOS_ERROR(std::invalid_argument, "Provided type points to an optional, not a variant.");

      return (sa.get_size() - 2);
   }

   uint32_t types_manager_common::get_optional_tag_offset(type_id tid)const
   {
      auto tc = tid.get_type_class();
      if( tc == type_id::optional_struct_type )
      {
         auto sa = get_size_align(tid.get_element_type());
         return sa.get_size();
      }

      if( tc != type_id::variant_or_optional_type )
         EOS_ERROR(std::invalid_argument, "Must provide a variant type to this function.");

      auto itr = types.begin() + tid.get_type_index();
      type_id::size_align sa(*itr);

      ++itr;
      if( *itr < type_id::variant_case_limit )
         EOS_ERROR(std::invalid_argument, "Provided type points to a variant, not an optional.");

      return (sa.get_size() - 1);
   }

   pair<type_id::index_t, field_metadata::sort_order> 
   types_manager_common::get_base_info(type_id::index_t struct_index)const
   {
      if( struct_index + 3 >= types.size() )
         EOS_ERROR(std::out_of_range, "Struct index is not valid.");

      auto itr = types.begin() + struct_index + 3;
      field_metadata::sort_order base_sort_order = ( !sorted_window::get(*itr) ? field_metadata::unsorted 
                                                                               : ( ascending_window::get(*itr) ? field_metadata::ascending
                                                                                                               : field_metadata::descending ) );

      return {index_window::get(*itr), base_sort_order};
   }

   pair<type_id, uint32_t> types_manager_common::get_container_element_type(type_id tid)const
   {
      uint32_t num_elements = 0;
      switch( tid.get_type_class() )
      {
         case type_id::small_array_of_builtins_type:
         case type_id::small_array_type:
            num_elements = tid.get_small_array_size();
            break;
         case type_id::array_type:
         {
            auto itr = types.begin() + tid.get_type_index() + 1;
            num_elements = *itr;
            ++itr;
            type_id element_tid(*itr);
            return {element_tid, num_elements};
         }
         case type_id::vector_type:
            break;
         case type_id::vector_of_something_type:
            break;
         case type_id::optional_struct_type:
            num_elements = 1;
            break;
         case type_id::variant_or_optional_type:
         {
            auto itr = types.begin() + tid.get_type_index() + 1;
            if( *itr >= type_id::variant_case_limit ) // Optional type
            {
               type_id element_tid(*itr);
               return {element_tid, 1};
            }
            EOS_ERROR(std::invalid_argument, "Given type is not a container.");
         }
         case type_id::builtin_type:
         {
            auto b = tid.get_builtin_type();
            if( b == type_id::builtin_string || b == type_id::builtin_bytes )
               return {type_id(type_id::builtin_uint8), 0};
            EOS_ERROR(std::invalid_argument, "Given type is not a container.");
         }
         case type_id::struct_type:
            EOS_ERROR(std::invalid_argument, "Given type is not a container.");
      }

      return {tid.get_element_type(), num_elements};
   }

   uint8_t  types_manager_common::get_num_indices_in_table(type_id::index_t index)const
   {
      if( index + 3 > types.size() )
         EOS_ERROR(std::invalid_argument, "Table index is not valid.");
 
      auto itr = types.begin() + index;
      return index_seq_window::get(*itr);
   }

   type_id::index_t types_manager_common::get_struct_index_of_table_object(type_id::index_t index)const
   {
      if( index + 3 > types.size() )
         EOS_ERROR(std::invalid_argument, "Table index is not valid.");
      
      auto itr = types.begin() + index;
      return index_window::get(*itr);
   }

   types_manager_common::table_index types_manager_common::get_table_index(type_id::index_t index, uint8_t index_seq_num)const
   {
      return {*this, index, index_seq_num};
   }

   types_manager_common::table_index::table_index(const types_manager_common& tm, type_id::index_t index, uint8_t index_seq_num)
      : tm(tm)
   {
      if( index + 1 + 2*(index_seq_num+1) > tm.types.size() )
         EOS_ERROR(std::invalid_argument, "Table index is not valid.");

      auto itr = tm.types.begin() + index;
      if( index_seq_window::get(*itr) <= index_seq_num )
         EOS_ERROR(std::out_of_range, "Index sequence number if outside the range of valid sequence numbers for this table.");

      itr = tm.types.begin() + index + 1 + 2*index_seq_num;

      index_info = *itr;
      ++itr;
      members_offset = *itr;
   }

   bool types_manager_common::table_index::is_unique()const
   {
      return unique_window::get(index_info);
   }

   bool types_manager_common::table_index::is_ascending()const
   {
      return ascending_window::get(index_info);
   }

   type_id types_manager_common::table_index::get_key_type()const
   {
      if( index_type_window::get(index_info) )
         return type_id::make_struct(index_window::get(index_info));
      else
         return type_id(static_cast<type_id::builtin>(index_window::get(index_info)));
   }

   range<vector<field_metadata>::const_iterator> 
   types_manager_common::table_index::get_sorted_members()const
   {
      uint32_t num_sorted_members = 1;
      if( index_type_window::get(index_info) )
         std::tie(std::ignore, num_sorted_members, std::ignore) = tm.get_members_common(index_window::get(index_info));
      
      return make_range(tm.members, members_offset, members_offset + num_sorted_members); 
   }

   template<typename B>
   inline
   typename std::enable_if<std::is_integral<B>::value, int8_t>::type
   compare_primitives(B lhs, B rhs)
   {
      if( lhs < rhs )
         return -1;
      else if( rhs < lhs )
         return 1;
      return 0;
   }

   struct compare_visitor
   {
      const types_manager_common& tm;
      const raw_region& lhs;
      const raw_region& rhs;
      uint32_t lhs_offset;
      uint32_t rhs_offset;
      bool ascending;
      int8_t comparison_result;

      compare_visitor(const types_manager_common& tm, 
                      const raw_region& lhs, uint32_t lhs_offset, 
                      const raw_region& rhs, uint32_t rhs_offset, bool ascending ) 
         : tm(tm), lhs(lhs), rhs(rhs), lhs_offset(lhs_offset), rhs_offset(rhs_offset), ascending(ascending), comparison_result(0)
      {}

      using traversal_shortcut = types_manager_common::traversal_shortcut;
      using array_type    = types_manager_common::array_type;
      using struct_type   = types_manager_common::struct_type;
      using vector_type   = types_manager_common::vector_type;
      using optional_type = types_manager_common::optional_type;
      using variant_type  = types_manager_common::variant_type; 

      template<typename T>
      traversal_shortcut operator()(const T&) { return types_manager_common::no_shortcut; } // Default implementation
      template<typename T, typename U>
      traversal_shortcut operator()(const T&, U) { return types_manager_common::no_shortcut; } // Default implementation

      traversal_shortcut operator()(type_id::builtin b)
      {
         bool is_string_or_bytes = false;
         switch( b )
         {         
            case type_id::builtin_int8:
               comparison_result = compare_primitives(lhs.get<int8_t>(lhs_offset), rhs.get<int8_t>(rhs_offset));
               break;
            case type_id::builtin_uint8:
               comparison_result = compare_primitives(lhs.get<uint8_t>(lhs_offset), rhs.get<uint8_t>(rhs_offset));
               break;
            case type_id::builtin_int16:
               comparison_result = compare_primitives(lhs.get<int16_t>(lhs_offset), rhs.get<int16_t>(rhs_offset));
               break;
            case type_id::builtin_uint16:
               comparison_result = compare_primitives(lhs.get<uint16_t>(lhs_offset), rhs.get<uint16_t>(rhs_offset));
               break;
            case type_id::builtin_int32:
               comparison_result = compare_primitives(lhs.get<int32_t>(lhs_offset), rhs.get<int32_t>(rhs_offset));
               break;
            case type_id::builtin_uint32:
               comparison_result = compare_primitives(lhs.get<uint32_t>(lhs_offset), rhs.get<uint32_t>(rhs_offset));
               break;
            case type_id::builtin_int64:
               comparison_result = compare_primitives(lhs.get<int64_t>(lhs_offset), rhs.get<int64_t>(rhs_offset));
               break;
            case type_id::builtin_uint64:
               comparison_result = compare_primitives(lhs.get<uint64_t>(lhs_offset), rhs.get<uint64_t>(rhs_offset));
               break;
            case type_id::builtin_bool:
               comparison_result = compare_primitives(lhs.get<bool>(lhs_offset << 3), rhs.get<bool>(rhs_offset << 3));
               break;
            case type_id::builtin_string:
            case type_id::builtin_bytes:
               is_string_or_bytes = true;
               break;
            case type_id::builtin_rational:
            {
               auto lhs_numerator   = lhs.get<int64_t>(lhs_offset);
               auto lhs_denominator = lhs.get<uint64_t>(lhs_offset+8);
               auto rhs_numerator   = rhs.get<int64_t>(rhs_offset);
               auto rhs_denominator = rhs.get<uint64_t>(rhs_offset+8);

               if( lhs_denominator == 0 && rhs_denominator == 0 )
               {
                  if( lhs_numerator < rhs_numerator )
                     comparison_result = -1;
                  else if( rhs_numerator < lhs_numerator )
                     comparison_result = 1;
               }
               else
               {
                  __int128 x = static_cast<__int128>(lhs_numerator) * rhs_denominator;
                  __int128 y = static_cast<__int128>(rhs_numerator) * lhs_denominator;

                  if( x < y )
                     comparison_result = -1;
                  else if( y < x )
                     comparison_result = 1;
               }
               break;
            }
            case type_id::builtin_any:
            {
               auto lhs_type = lhs.get<uint32_t>(lhs_offset);
               auto lhs_data_offset = lhs.get<uint32_t>(lhs_offset+4);
               auto rhs_type = rhs.get<uint32_t>(rhs_offset);
               auto rhs_data_offset = rhs.get<uint32_t>(rhs_offset+4);

               if( lhs_type == 0 && rhs_type != 0 )
               {
                  type_id(rhs_type); // Validate type
                  comparison_result = -1;
               }
               else if( lhs_type != 0 && rhs_type == 0 )
               {
                  type_id(lhs_type); // Validate type
                  comparison_result = 1;
               }
               else if( lhs_type != 0 && rhs_type != 0 )
               {
                  auto lhs_tid = type_id(lhs_type); // Validate type
                  auto rhs_tid = type_id(rhs_type); // Validate type
                  if( lhs_tid < rhs_tid )
                     comparison_result = -1;
                  else if( rhs_tid < lhs_tid )
                     comparison_result = 1;
                  else // Both Any type instances have the same dynamic type
                  {
                     compare_visitor v(tm, lhs, lhs_data_offset, rhs, rhs_data_offset, true);
                     tm.traverse_type(lhs_tid, v); 
                     comparison_result = v.comparison_result;
                  }
               }
               break;
            }
         }

         if( is_string_or_bytes )
         {
            auto lhs_num_elements = lhs.get<uint32_t>(lhs_offset);
            auto rhs_num_elements = rhs.get<uint32_t>(rhs_offset);
            auto num_elements = std::min(lhs_num_elements, rhs_num_elements);

            lhs_offset = lhs.get<uint32_t>(lhs_offset+4);
            rhs_offset = rhs.get<uint32_t>(rhs_offset+4);
            for( uint32_t i = 0; i < num_elements; ++i )
            {
               comparison_result = compare_primitives(lhs.get<uint8_t>(lhs_offset), rhs.get<uint8_t>(rhs_offset));

               if( comparison_result != 0 )
                  break;

               ++lhs_offset;
               ++rhs_offset;
            }

            if( comparison_result == 0 )
            {
               if( lhs_num_elements < rhs_num_elements )
                  comparison_result = -1;
               else if( lhs_num_elements > rhs_num_elements )
                  comparison_result = 1;
            }
         }

         comparison_result = (ascending ? comparison_result : -comparison_result);

         return types_manager_common::no_shortcut; 
      }

      traversal_shortcut operator()(struct_type t)
      {
         for( auto f : tm.get_sorted_members(t.index) )
         {
            auto f_offset = f.get_offset();
            compare_visitor v(tm, lhs, lhs_offset + f_offset, rhs, rhs_offset + f_offset, f.get_sort_order() == field_metadata::ascending);
            tm.traverse_type(f.get_type_id(), v);
            if( v.comparison_result != 0 )
            {
               comparison_result = (ascending ? v.comparison_result : -v.comparison_result);
               break;
            }
         }

         return types_manager_common::no_deeper;
      }
      
      traversal_shortcut operator()(array_type t) 
      {
         auto sa = tm.get_size_align(t.element_type);
         auto stride = sa.get_stride();
         compare_visitor v(tm, lhs, lhs_offset, rhs, rhs_offset, ascending);
         for( uint32_t i = 0; i < t.num_elements; ++i )
         {
            tm.traverse_type(t.element_type, v);
            if( v.comparison_result != 0 )
            {
               comparison_result = v.comparison_result;
               break;
            }
            v.lhs_offset += stride;
            v.rhs_offset += stride;
         }
       
         return types_manager_common::no_deeper; 
      }

      traversal_shortcut operator()(vector_type t) 
      {
         auto sa = tm.get_size_align(t.element_type);
         auto stride = sa.get_stride();

         auto lhs_num_elements = lhs.get<uint32_t>(lhs_offset);
         auto rhs_num_elements = rhs.get<uint32_t>(rhs_offset);
         auto num_elements = std::min(lhs_num_elements, rhs_num_elements);

         compare_visitor v(tm, lhs, lhs.get<uint32_t>(lhs_offset+4), rhs, rhs.get<uint32_t>(rhs_offset+4), ascending);
         for( uint32_t i = 0; i < num_elements; ++i )
         {
            tm.traverse_type(t.element_type, v);
            if( v.comparison_result != 0 )
            {
               comparison_result = v.comparison_result;
               break;
            }
            v.lhs_offset += stride;
            v.rhs_offset += stride;
         }

         if( comparison_result == 0 )
         {
            if( lhs_num_elements < rhs_num_elements )
               comparison_result = (ascending ? -1 : 1);
            else if( lhs_num_elements > rhs_num_elements )
               comparison_result = (ascending ? 1 : -1);
         }         
         
         return types_manager_common::no_deeper; 
      }
      
      traversal_shortcut operator()(optional_type t) 
      {
         bool lhs_exists = lhs.get<bool>((lhs_offset + t.tag_offset) << 3);
         bool rhs_exists = rhs.get<bool>((rhs_offset + t.tag_offset) << 3);

         if( lhs_exists && !rhs_exists )
            comparison_result = (ascending ? 1 : -1);
         else if( !lhs_exists && rhs_exists )
            comparison_result = (ascending ? -1 : 1);
         else if( lhs_exists && rhs_exists )
            tm.traverse_type(t.element_type, *this);
         
         return types_manager_common::no_deeper; 
      }
 
      traversal_shortcut operator()(variant_type t) 
      {
         auto tid = type_id::make_variant(t.index);
         auto tag_offset = tm.get_variant_tag_offset(tid);
         
         auto lhs_which = lhs.get<uint16_t>(lhs_offset + tag_offset);
         auto rhs_which = rhs.get<uint16_t>(rhs_offset + tag_offset);

         if( lhs_which < rhs_which )
            comparison_result = (ascending ? -1 : 1);
         else if( lhs_which > rhs_which )
            comparison_result = (ascending ? 1 : -1);
         else
            tm.traverse_type(tm.get_variant_case_type(tid, t.index), *this);

         return types_manager_common::no_deeper;
      }
     
      traversal_shortcut operator()() 
      {
         EOS_ERROR(std::runtime_error, "Invariant failure: Void type should not be allowed in structs or table keys.");
         return types_manager_common::no_shortcut; // Should never be reached. Just here to silence compiler warning.
      }
   };

   int8_t types_manager_common::compare_object_with_key(const raw_region& object_data, const raw_region& key_data, const table_index& ti)const
   {
      auto object_range = ti.get_sorted_members();
      auto key_type = ti.get_key_type();
      auto tc = key_type.get_type_class();
      if( tc == type_id::builtin_type )
      {
         auto f = *(object_range.begin());

         if( f.get_type_id() != key_type )
            EOS_ERROR(std::runtime_error, "Invariant failure: key type of index does not match the type of the view into the object.");

         compare_visitor v(*this, object_data, f.get_offset(), key_data, 0, f.get_sort_order() == field_metadata::ascending);
         traverse_type( key_type, v);
         return (ti.is_ascending() ? v.comparison_result : -v.comparison_result);
      }
      else if(tc != type_id::struct_type )
         EOS_ERROR(std::runtime_error, "Invariant failure: key type of index is not builtin type or struct.");

      auto key_range = ti.get_types_manager().get_sorted_members(key_type.get_type_index());
      auto num_sorted_members = key_range.end() - key_range.begin();
      if( num_sorted_members != (object_range.end() - object_range.begin()) )
         EOS_ERROR(std::runtime_error, "Invariant failure: Number of sorted members of key and key-shaped view into the object are not the same.");

      for( auto i = 0; i < num_sorted_members; ++i )
      {
         auto f     = *(object_range.begin() + i);
         auto f_key = *(key_range.begin() + i); 

         auto f_offset     = f.get_offset();
         auto f_key_offset = f_key.get_offset();

         f.set_offset(0);
         f_key.set_offset(0);

         if( f != f_key )
            EOS_ERROR(std::runtime_error, "Invariant failure: key and key-shaped view into the object are structs with different member metadata.");

         compare_visitor v(*this, object_data, f_offset, key_data, f_key_offset, f.get_sort_order() == field_metadata::ascending);
         traverse_type(f.get_type_id(), v);
         if( v.comparison_result != 0)
            return (ti.is_ascending() ? v.comparison_result : -v.comparison_result);
      }

      return 0;
   }

   int8_t types_manager_common::compare_objects(uint64_t lhs_id, const raw_region& lhs, uint64_t rhs_id, const raw_region& rhs, const table_index& ti)const
   {
      int8_t comparison_result = 0;
      
      for( auto f : ti.get_sorted_members() )
      {
         auto f_offset = f.get_offset();
         compare_visitor v(*this, lhs, f_offset, rhs, f_offset, f.get_sort_order() == field_metadata::ascending);
         traverse_type(f.get_type_id(), v);
         if( v.comparison_result != 0 )
         {
            comparison_result = (ti.is_ascending() ? v.comparison_result : -v.comparison_result);
            break;
         }
      }

      if( comparison_result == 0 && !ti.is_unique() )
      {
         if( lhs_id < rhs_id )
            comparison_result = -1;
         else if( rhs_id > lhs_id )
            comparison_result = 1;
      }

      return comparison_result;
   }

} }

