#include <eos/eoslib/full_types_manager.hpp>
#include <eos/eoslib/exceptions.hpp>

#ifdef EOS_TYPES_FULL_CAPABILITY
#include <ostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <boost/io/ios_state.hpp>
#endif

namespace eos { namespace types {

   range<typename vector<field_metadata>::const_iterator> 
   full_types_manager::get_all_members(type_id::index_t struct_index)const
   {
      uint32_t member_data_offset;
      uint16_t num_sorted_members;
      uint16_t total_num_members;
      std::tie(member_data_offset, num_sorted_members, total_num_members) = get_members_common(struct_index);
      return make_range(members, member_data_offset + num_sorted_members, (member_data_offset + num_sorted_members) + total_num_members);
   }

   field_metadata full_types_manager::get_member(type_id::index_t struct_index, uint16_t member_index)const
   {
      uint32_t member_data_offset;
      uint16_t num_sorted_members;
      uint16_t total_num_members;
      std::tie(member_data_offset, num_sorted_members, total_num_members) = get_members_common(struct_index);
      if( member_index >= total_num_members )
         EOS_ERROR(std::out_of_range, "Trying to get a member which does not exist.");

      return members[(member_data_offset + num_sorted_members) + member_index];
   }

   type_id::index_t full_types_manager::get_table(const string& name)const
   {
      auto itr = lookup_by_name.find(name);
      if( itr == lookup_by_name.end() )
         EOS_ERROR(std::invalid_argument, "Cannot find table with the given name.");

      auto storage = valid_indices[itr->second];
      if( static_cast<index_type>(index_type_window::get(storage)) != index_type::table_index )
         EOS_ERROR(std::invalid_argument, "Name does not refer to a table.");

      return index_window::get(storage);
   }

   type_id::index_t full_types_manager::get_struct_index(const string& name)const
   {
      auto itr = lookup_by_name.find(name);
      if( itr == lookup_by_name.end() )
         EOS_ERROR(std::invalid_argument, "Cannot find struct with the given name.");

      auto storage = valid_indices[itr->second];
      auto t = static_cast<index_type>(index_type_window::get(storage));
      if( t == index_type::simple_struct_index || t == index_type::derived_struct_index )
        return index_window::get(storage);

      storage = valid_indices[large_index_window::get(valid_indices[itr->second + 1])];

      return index_window::get(storage);
   }

   string full_types_manager::get_struct_name(type_id::index_t index)const
   {
      auto itr = find_index(index);
      if( itr == valid_indices.end() )
         EOS_ERROR(std::invalid_argument, "Not a valid struct index.");
      
      auto t = static_cast<index_type>(index_type_window::get(*itr));
      if( t != index_type::simple_struct_index && t != index_type::derived_struct_index )
         EOS_ERROR(std::invalid_argument, "Index is not to a struct type.");

      ++itr; 

      return (lookup_by_name.begin() + index_window::get(*itr))->first;
   }

   bool full_types_manager::is_type_valid(type_id tid)const
   {
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
         case type_id::optional_struct_type:
            return is_type_valid(tid.get_element_type());
         default:
            break;
      }

      auto index = tid.get_type_index();
      if( index >= types.size() )
         return false;

      auto itr = find_index(index);
      if( itr == valid_indices.end() )
         return false;

      switch( static_cast<index_type>(index_type_window::get(*itr)) )
      {
         case index_type::table_index:
            break;
         case index_type::simple_struct_index:
         case index_type::derived_struct_index:
         case index_type::tuple_index:
            return (tc == type_id::struct_type);
         case index_type::array_index:
            return (tc == type_id::array_type);
         case index_type::vector_index:
            return (tc == type_id::vector_type);
         case index_type::sum_type_index:
            return (tc == type_id::variant_or_optional_type);
      }
      
      return false;
   }

   type_id::size_align full_types_manager::get_size_align(type_id tid)const
   {
      if( !is_type_valid(tid) )
         EOS_ERROR(std::invalid_argument, "Type is not valid.");

      return types_manager_common::get_size_align(tid);
   }

   pair<const string&, int16_t> 
   full_types_manager::get_struct_info(vector<uint32_t>::const_iterator itr)const
   {
      if( itr == valid_indices.end() || continuation_window::get(*itr) )
         EOS_ERROR(std::invalid_argument, "Iterator does not point to the start of an entry within valid_indices.");

      auto t = static_cast<index_type>(index_type_window::get(*itr));
      if( t != index_type::simple_struct_index && t != index_type::derived_struct_index )
         EOS_ERROR(std::invalid_argument, "Iterator points to an entry that is not for a struct.");

      ++itr;
      const string& name = (lookup_by_name.begin() + index_window::get(*itr))->first;
      ++itr;
      ++itr;
      
      return {name, base_sort_window::get(*itr)};
   }

   range<vector<uint64_t>::const_iterator> 
   full_types_manager::get_struct_fields_info(vector<uint32_t>::const_iterator itr)const
   {
      if( itr == valid_indices.end() || continuation_window::get(*itr) )
         EOS_ERROR(std::invalid_argument, "Iterator does not point to the start of an entry within valid_indices.");

      auto t = static_cast<index_type>(index_type_window::get(*itr));
      if( t != index_type::simple_struct_index && t != index_type::derived_struct_index )
         EOS_ERROR(std::invalid_argument, "Iterator points to an entry that is not for a struct.");

      uint64_t fields_info_index = 0;
      uint16_t num_fields = 0;

      fields_info_index |= (static_cast<uint64_t>(three_bits_window::get(*itr)) << 37);

      ++itr;
      fields_info_index |= (static_cast<uint64_t>(seven_bits_window::get(*itr)) << 30);

      ++itr;
      fields_info_index |= (static_cast<uint64_t>(fifteen_bits_window::get(*itr)) << 15);
      num_fields = field_size_window::get(*itr);

      ++itr;
      fields_info_index |= static_cast<uint64_t>(fifteen_bits_window::get(*itr));

      return make_range(fields_info, fields_info_index, fields_info_index + num_fields);
   }

   std::ptrdiff_t full_types_manager::adjust_iterator(vector<uint32_t>::const_iterator& itr)const
   {
      std::ptrdiff_t seq = 0;

      if( itr == valid_indices.end() )
         return seq;

      while( continuation_window::get(*itr) )
      {
         if( itr == valid_indices.begin() )
            EOS_ERROR(std::runtime_error, "Invariant failure: valid_indices was not constructed properly.");
         --itr;
         --seq;
      }
      
      return seq;
   }

   std::ptrdiff_t full_types_manager::advance_iterator(vector<uint32_t>::const_iterator& itr)const
   {
      std::ptrdiff_t step = 1;
      switch( static_cast<index_type>(index_type_window::get(*itr)) )
      {
         case index_type::table_index:
            step = 2;
            break;
         case index_type::simple_struct_index:
         case index_type::derived_struct_index:
            step = 4;
            break;
         default:
            break;
      }
      std::advance(itr, step);
      return step;
   }

   vector<uint32_t>::const_iterator full_types_manager::find_index(type_id::index_t index)const
   {
      auto first = valid_indices.begin();
      auto itr   = valid_indices.end();
      size_t step = 0;
      size_t count = std::distance(first, itr);

      while( count > 0 )
      {
         itr = first; 
         step = count / 2;

         std::advance(itr, step); 
         step += adjust_iterator(itr);

         if( index_window::get(*itr) < index )
         {
            count -= step + advance_iterator(itr);
            first = itr; 
         }
         else
            count = step;
      }

      if( index_window::get(*itr) == index )
         return itr;
      else
         return valid_indices.end();
   }

#ifdef EOS_TYPES_FULL_CAPABILITY
   
   struct print_type_visitor
   {
      const full_types_manager& tm;
      std::ostream&            os;
      bool start_of_variant;

      print_type_visitor(const full_types_manager& tm, std::ostream& os) 
         : tm(tm), os(os), start_of_variant(false)
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
         os << type_id::get_builtin_type_name(b);
         return types_manager_common::no_shortcut; 
      }

      traversal_shortcut operator()(struct_type t)
      {
         auto itr = tm.find_index(t.index);
         auto indx_type = static_cast<full_types_manager::index_type>(full_types_manager::index_type_window::get(*itr));

         if( indx_type == full_types_manager::index_type::tuple_index ) // If tuple
         {
            auto r = tm.get_all_members(t.index);
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
               tm.traverse_type(itr->get_type_id(), *this);
            }

            os << ">";
            return types_manager_common::no_deeper;
         }

         ++itr; // Otherwise it is a struct. Now itr is point to the second item within the struct entry in valid_indices
     
         os << (tm.lookup_by_name.begin() + full_types_manager::index_window::get(*itr))->first;
         return types_manager_common::no_deeper;
      }
      
      traversal_shortcut operator()(variant_type t) 
      {
         os << "Variant<";
         start_of_variant = true;
         return types_manager_common::no_shortcut;
      }

      traversal_shortcut operator()(variant_type t, uint32_t case_index) 
      {
         if( start_of_variant )
            start_of_variant = false;
         else
            os << ", ";

         return types_manager_common::no_shortcut;
      }

      traversal_shortcut operator()(variant_type t, bool) 
      {
         os << ">";
         return types_manager_common::no_shortcut;
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
            tm.traverse_type(t.element_type, *this);
            os << ", " << t.num_elements;
         }
         os << ">";
       
         return types_manager_common::no_deeper; 
      }

      traversal_shortcut operator()(vector_type t) 
      {
         os << "Vector<";
         tm.traverse_type(t.element_type, *this);
         os << ">";
         return types_manager_common::no_deeper; 
      }
      
      traversal_shortcut operator()(optional_type t) 
      {
         os << "Optional<";
         tm.traverse_type(t.element_type, *this);
         os << ">";  
         return types_manager_common::no_deeper; 
      }
      
      traversal_shortcut operator()() 
      {
         os << "Void";
         return types_manager_common::no_shortcut;
      }
   };


   void full_types_manager::print_type(std::ostream& os, type_id tid)const
   {
      boost::io::ios_flags_saver ifs( os );

      print_type_visitor v(*this, os);
      
      os << std::dec << std::setfill(' ');
      static const char* tab        = "    ";
      static const char* ascending  = "ascending";
      static const char* descending = "descending";
    
      if( tid.get_type_class() == type_id::struct_type )
      {
         auto struct_index = tid.get_type_index();
         auto struct_itr = find_index(struct_index);
 
         auto t = index_type::table_index; // Set to anything other than simple_struct_index, derived_struct_index, or tuple_index. 
         if( struct_itr != valid_indices.end() )
            t = static_cast<index_type>(index_type_window::get(*struct_itr)); // Set to actual index type.

         switch( t )
         {
            case index_type::simple_struct_index:
            case index_type::derived_struct_index:
            {
               auto info = get_struct_info(struct_itr);
               auto res = get_base_info(struct_index);
               bool base_exists = (res.first != type_id::type_index_limit);

               os << "struct " << info.first;
               if( base_exists )
               {
                  os << " : ";
                  os << get_struct_name(res.first);
                  if( res.second != field_metadata::unsorted )
                  {
                     os << " // Base sorted in " << ((res.second == field_metadata::ascending) ? ascending : descending);
                     os << " order (priority " << std::abs(info.second) << ")";
                  }
               }
               os << std::endl << "{" << std::endl;

               auto r = get_all_members(struct_index);
               auto begin = r.begin() + (base_exists ? 1 : 0 );
               auto end   = r.end();
               auto r2 = get_struct_fields_info(struct_itr);
               auto info_itr = r2.begin();
 
               std::ostringstream oss;
               print_type_visitor v2(*this, oss);
              
               size_t max_typename_length = 0;
               size_t max_fieldname_length = 0;
               vector<pair<string, size_t>> field_print_info;
               for( auto itr = begin; itr != end; ++info_itr, ++itr )
               {
                  if( info_itr == r2.end() )
                     EOS_ERROR(std::runtime_error, "Invariant failure: get_struct_fields_info return range of unexpected size.");

                  auto field_length = field_names[fields_index_window::get(*info_itr)].size();
                  max_fieldname_length = std::max(max_fieldname_length, field_length);

                  traverse_type(itr->get_type_id(), v2);
                  field_print_info.emplace_back(oss.str(), field_length);
                  auto type_length = oss.str().size();
                  oss.str("");

                  max_typename_length  = std::max(max_typename_length, type_length);
               }

               info_itr = r2.begin();
               for( auto itr = field_print_info.begin(); itr != field_print_info.end(); ++info_itr, ++itr)
               {
                  os << tab << std::left << std::setw(max_typename_length);
                  os << itr->first << " ";
                  os << field_names[fields_index_window::get(*info_itr)] << ";";

                  auto so = sort_order_window::get(*info_itr);
                  if( so != 0 )
                  {
                     os << std::right << std::setw(max_fieldname_length - itr->second) << "";
                     os << " // Sorted in " << (so > 0 ? ascending : descending) 
                        << " order (priority " << std::abs(so) << ")";
                  }
                  os << std::endl;
               }
               os << "};" << std::endl;

               return;
            }
            case index_type::tuple_index:
               break; // Tuple needs to be handled by the same code that handles other non-struct types.
            default:
               EOS_ERROR(std::invalid_argument, "Struct or tuple does not exist with index specified in the given type.");
         }
      }
      else if( !tid.is_void() && !is_type_valid(tid) )
         EOS_ERROR(std::logic_error, "Type is not valid.");

      // Otherwise, print the non-struct type (includes tuples):
      traverse_type(tid, v);
   }

#endif

} }

