#include <eos/types/full_types_manager.hpp>

#include <ostream>
#include <iomanip>
#include <boost/io/ios_state.hpp>

namespace eos { namespace types {

   range<typename vector<field_metadata>::const_iterator> 
   full_types_manager::get_all_members(type_id::index_t struct_index)const
   {
      uint32_t member_data_offset;
      uint32_t num_sorted_members;
      uint32_t total_num_members;
      std::tie(member_data_offset, num_sorted_members, total_num_members) = get_members_common(struct_index);
      return make_range(members, member_data_offset + num_sorted_members, member_data_offset + num_sorted_members + total_num_members);
   }

   field_metadata full_types_manager::get_member(type_id::index_t struct_index, uint16_t member_index)const
   {
      uint32_t member_data_offset;
      uint32_t num_sorted_members;
      uint32_t total_num_members;
      std::tie(member_data_offset, num_sorted_members, total_num_members) = get_members_common(struct_index);
      if( member_index >= total_num_members )
         throw std::out_of_range("Trying to get a member which does not exist.");

      return members[member_data_offset + num_sorted_members + member_index];
   }

   type_id::index_t full_types_manager::get_table(const string& name)const
   {
      auto itr = lookup_by_name.find(name);
      if( itr == lookup_by_name.end() || !is_table_window::get(itr->second) )
         throw std::invalid_argument("Cannot find table with the given name.");

      return index_window::get(itr->second);
   }

   type_id::index_t full_types_manager::get_struct_index(const string& name)const
   {
      auto itr = lookup_by_name.find(name);
      if( itr == lookup_by_name.end() )
         throw std::invalid_argument("Cannot find struct with the given name.");

      auto index = index_window::get(itr->second);

      if( !is_table_window::get(itr->second) )
         return index;

      return get_struct_index_of_table_object(index); 
   }

   string full_types_manager::get_struct_name(type_id::index_t index)const
   {

      auto itr = valid_indices.find(index);
      if( itr == valid_indices.end() || itr->second.is_void() || itr->second.get_type_class() != type_id::struct_type )
         throw std::invalid_argument("Not a valid struct index.");

      auto i = itr->second.get_type_index();
      if( i == 0 ) // Means the entry in valid_indices represents a tuple.
         throw std::invalid_argument("Index is to a tuple which does not have a name.");

      --i; // Now i is the index into the sorted sequence of pairs making up the lookup_by_name flat_map.
      
      if( i >= lookup_by_name.size() )
         throw std::runtime_error("Invariant failure: the lookup_by_name and/or valid_indices maps were not created properly.");

      return (lookup_by_name.begin() + i)->first;
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
            return is_type_valid(tid.get_element_type());
         default:
            break;
      }

      auto index = tid.get_type_index();
      if( index >= types.size() )
         return false;

      auto itr = valid_indices.find(index);
      if( itr == valid_indices.end() )
         return false;

      if( itr->second.is_void() ) // Void value in valid_indices means the entry represents a table.
         return false;

      return (itr->second.get_type_class() == tc);
   }

   type_id::size_align full_types_manager::get_size_align(type_id tid)const
   {
      if( !is_type_valid(tid) )
         throw std::invalid_argument("Type is not valid.");

      return types_manager_common::get_size_align(tid);
   }

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
         auto i = tm.valid_indices.find(t.index)->second.get_type_index();

         if( i == 0 ) // If tuple
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

         --i; // Otherwise it is a struct, and at this point i is the index into the sorted sequence of pairs making up the lookup_by_name flat_map.
     
         if( i >= tm.lookup_by_name.size() )
            throw std::runtime_error("Invariant failure: the lookup_by_name and/or valid_indices maps were not created properly.");

         os << (tm.lookup_by_name.begin() + i)->first;
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
      
      os << std::dec;
      static const char* tab        = "    ";
      static const char* ascending  = "ascending";
      static const char* descending = "descending";
    
      if( tid.get_type_class() == type_id::struct_type )
      {
         auto struct_index = tid.get_type_index();
         auto struct_itr = valid_indices.find(struct_index);
 
         if( struct_itr == valid_indices.end() || struct_itr->second.is_void() || struct_itr->second.get_type_class() != type_id::struct_type )
            throw std::invalid_argument("Struct or tuple does not exist with index specified in the given type.");
      
         auto i = struct_itr->second.get_type_index();
         if( i != 0 ) // Type is a struct
         {
            --i; // Now i is the index into the sorted sequence of pairs making up the lookup_by_name flat_map.
      
            if( i >= lookup_by_name.size() )
               throw std::runtime_error("Invariant failure: the lookup_by_name and/or valid_indices maps were not created properly.");

            const auto& struct_name = (lookup_by_name.begin() + i)->first;
            auto res = get_base_info(struct_index);
            bool base_exists = (res.first != type_id::type_index_limit);

            os << "struct " << struct_name;
            if( base_exists )
            {
               os << " : ";
               os << get_struct_name(res.first);
               if( res.second != field_metadata::unsorted )
               {
                  os << " // Base sorted in " << ((res.second == field_metadata::ascending) ? ascending : descending);
                  os << " order";
               }
            }
            os << std::endl << "{" << std::endl;

            auto r = get_all_members(struct_index);
            uint32_t field_seq_num = 0;
            for( auto itr = r.begin() + (base_exists ? 1 : 0); itr != r.end(); ++field_seq_num, ++itr)
            {
               os << tab;
               traverse_type(itr->get_type_id(), v);
               os << " f" << field_seq_num << ";";

               auto so = itr->get_sort_order();
               if( so != field_metadata::unsorted )
                  os << " // Sorted in " << (so == field_metadata::ascending ? ascending : descending) << " order";
               os << std::endl;
            }
            os << "};" << std::endl;

            return;
         }
         // Otherwise it is a tuple and so it needs to be handled by the same code that handles other non-struct types.
      }
      else if( !tid.is_void() && !is_type_valid(tid) )
         throw std::logic_error("Type is not valid.");

      // Otherwise, print the non-struct type (includes tuples):
      traverse_type(tid, v);
   }

} }

