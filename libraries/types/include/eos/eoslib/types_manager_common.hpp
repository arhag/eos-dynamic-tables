#pragma once

#include <eos/eoslib/field_metadata.hpp>
#include <eos/eoslib/raw_region.hpp>

#include <utility>
#include <tuple>
#include <vector>

namespace eos { namespace types {

   using std::pair;
   using std::tuple;
   using std::vector;

   template <class Iter>
   class range {
       Iter b;
       Iter e;
   public:

       range(Iter b, Iter e) : b(b), e(e) {}

       Iter begin() { return b; }
       Iter end() { return e; }
   };

   template <class Container>
   range<typename Container::iterator> 
   make_range(Container& c, size_t b, size_t e) {
       return range<typename Container::iterator> (c.begin()+b, c.begin()+e);
   }

   template <class Container>
   range<typename Container::const_iterator> 
   make_range(const Container& c, size_t b, size_t e) {
       return range<typename Container::const_iterator> (c.begin()+b, c.begin()+e);
   }

   class types_manager_common
   {
   public:

      using sorted_window     = bit_view<bool,     31,  1, uint32_t>;
      using ascending_window  = bit_view<bool,     30,  1, uint32_t>;
      using unique_window     = bit_view<bool,     29,  1, uint32_t>;
      using index_type_window = bit_view<bool,     24,  1, uint32_t>;
      using index_window      = bit_view<uint32_t,  0, 24, uint32_t>;

      //using depth_window      = bit_view<uint8_t,  24,  6, uint32_t>;
      using index_seq_window  = bit_view<uint8_t,  24,  8, uint32_t>;


      class table_index
      {
      public:

         bool    is_unique()const;
         bool    is_ascending()const;
         type_id get_key_type()const;
         range<vector<field_metadata>::const_iterator> get_sorted_members()const;

         inline const types_manager_common& get_types_manager()const { return tm; }

         friend class types_manager_common;

      private:
         const types_manager_common& tm;
         uint32_t index_info;
         uint32_t members_offset;
         
         table_index(const types_manager_common& tm, type_id::index_t index, uint8_t index_seq_num);
      };

      types_manager_common(const types_manager_common& other) = delete;
      types_manager_common(types_manager_common&& other) = delete;

      types_manager_common(const vector<uint32_t>& types, const vector<field_metadata>& members)
         : types(types), members(members)
      {
      }
 
      inline type_id::size_align                         get_size_align(type_id tid)const { return get_size_align(tid, nullptr); }

      range<vector<field_metadata>::const_iterator>      get_sorted_members(type_id::index_t struct_index)const;

      type_id                                            get_variant_case_type(type_id tid, uint16_t which)const;
      uint32_t                                           get_variant_tag_offset(type_id tid)const;
      uint32_t                                           get_optional_tag_offset(type_id tid)const;
      pair<type_id::index_t, field_metadata::sort_order> get_base_info(type_id::index_t struct_index)const;
      pair<type_id, uint32_t>                            get_container_element_type(type_id tid)const;

      uint8_t                                            get_num_indices_in_table(type_id::index_t index)const;
      type_id::index_t                                   get_struct_index_of_table_object(type_id::index_t index)const;
      table_index                                        get_table_index(type_id::index_t index, uint8_t index_seq_num)const;

      int8_t                                             compare_object_with_key(const raw_region& object_data, const raw_region& key_data, const table_index& ti)const;
      int8_t                                             compare_objects(uint64_t lhs_id, const raw_region& lhs, uint64_t rhs_id, const raw_region& rhs, const table_index& ti)const;

      friend class table_index;
      friend class types_constructor;

      // Type traversal: (Caller needs to ensure types of types_manager_common is in a coherent state, or else things will go horribly wrong)
 
      enum traversal_shortcut
      {
         no_shortcut,
         no_deeper,
         return_now
      };
 
      struct struct_type   { type_id::index_t index; };
      struct array_type    { type_id element_type; uint32_t num_elements; };
      struct vector_type   { type_id element_type;   };
      struct optional_type { type_id element_type; uint32_t tag_offset; };
      struct variant_type  { type_id::index_t index; };
   
      template<typename Visitor>
      traversal_shortcut traverse_type(type_id tid, Visitor& v)const
      {
         if( tid.is_void() )
         {
            traversal_shortcut s = v();
            return (s == no_deeper ? no_shortcut : s);
         }

         switch( tid.get_type_class() )
         {
            case type_id::builtin_type:
            {
               traversal_shortcut s = v(tid.get_builtin_type());
               return (s == no_deeper ? no_shortcut : s);
            }
            case type_id::struct_type:
            {
               traversal_shortcut s = v(struct_type{tid.get_type_index()});
               return (s == no_deeper ? no_shortcut : s);
            }
            case type_id::vector_type:
            {
               //type_id element_type(types[tid.get_type_index()], true);
               type_id element_type(types[tid.get_type_index()]);
               traversal_shortcut traverse_element = v(vector_type{element_type});
               switch( traverse_element )
               {
                  case return_now:
                     return return_now;
                  case no_deeper:
                     return no_shortcut;
                  case no_shortcut:
                     break;
               }
               traversal_shortcut s = traverse_type(element_type, v);
               return (s == no_deeper ? no_shortcut : s);
            }
            case type_id::vector_of_something_type:
            {
               auto element_type = tid.get_element_type();
               traversal_shortcut traverse_element = v(vector_type{element_type});
               switch( traverse_element )
               {
                  case return_now:
                     return return_now;
                  case no_deeper:
                     return no_shortcut;
                  case no_shortcut:
                     break;
               }
               traversal_shortcut s = traverse_type(element_type, v);
               return (s == no_deeper ? no_shortcut : s);
            } 
            case type_id::optional_struct_type:
            {
               auto element_type = tid.get_element_type();

               traversal_shortcut traverse_element = v(optional_type{element_type, get_optional_tag_offset(tid)});
               switch( traverse_element )
               {
                  case return_now:
                     return return_now;
                  case no_deeper:
                     return no_shortcut;
                  case no_shortcut:
                     break;
               }
               traversal_shortcut s = traverse_type(element_type, v);
               return (s == no_deeper ? no_shortcut : s);
            }
            case type_id::variant_or_optional_type:
            {
               auto index = tid.get_type_index();
               if( types[index+1] >= type_id::variant_case_limit ) // If actually an optional
               {
                  //type_id element_type(types[index+1], true);
                  type_id element_type(types[index+1]);
                  traversal_shortcut traverse_element = v(optional_type{element_type, get_optional_tag_offset(tid)});
                  switch( traverse_element )
                  {
                     case return_now:
                        return return_now;
                     case no_deeper:
                        return no_shortcut;
                     case no_shortcut:
                        break;
                  }
                  traversal_shortcut s = traverse_type(element_type, v);
                  return (s == no_deeper ? no_shortcut : s);
               }

               // Otherwise, it is a variant:
               auto var = variant_type{index};
               traversal_shortcut traverse_variant = v(var);
               bool visit_cases = true;
               switch( traverse_variant )
               {
                  case return_now:
                     return return_now;
                  case no_deeper:
                     visit_cases = false;
                     break;
                  case no_shortcut:
                     break;
               }

               auto itr = types.begin() + index + 1;
               auto n = *itr;
               ++itr;
               if( visit_cases )
               {
                  for( uint32_t i = 0; i < n; ++i, ++itr )
                  {
                     //type_id t(*itr, true);
                     type_id t(*itr);
                     traversal_shortcut s = v(var, i);
                     switch( s )
                     {
                        case return_now:
                           return return_now;
                        case no_deeper:
                           visit_cases = false;
                        case no_shortcut:
                           break;
                     }
                     if( !visit_cases )
                        break;
                     s = traverse_type(t, v);
                     switch( s )
                     {
                        case return_now:
                           return return_now;
                        case no_deeper:
                        case no_shortcut:
                           break;
                     }
                  }
               }
               traversal_shortcut s = v(var, visit_cases);
               switch( s )
               {
                  case return_now:
                     return return_now;
                  case no_deeper:
                  case no_shortcut:
                     break;
               }
               return no_shortcut;
            }
            case type_id::small_array_of_builtins_type:
            case type_id::small_array_type: 
            {
               auto num_elems    = tid.get_small_array_size(); 
               auto element_type = tid.get_element_type();
               traversal_shortcut traverse_element = v(array_type{element_type, num_elems});
               switch( traverse_element )
               {
                  case return_now:
                     return return_now;
                  case no_deeper:
                     return no_shortcut;
                  case no_shortcut:
                     break;
               }
               traversal_shortcut s = traverse_type(element_type, v);
               return (s == no_deeper ? no_shortcut : s);
            }
            case type_id::array_type:
            {
               auto itr = types.begin() + tid.get_type_index() + 1;
               auto num_elems = *itr;
               ++itr;
               //type_id element_type(*itr, true);
               type_id element_type(*itr);
               traversal_shortcut traverse_element = v(array_type{element_type, num_elems});
               switch( traverse_element )
               {
                  case return_now:
                     return return_now;
                  case no_deeper:
                     return no_shortcut;
                  case no_shortcut:
                     break;
               }
               traversal_shortcut s = traverse_type(element_type, v);
               return (s == no_deeper ? no_shortcut : s);
            }
         }
         return no_shortcut; // Should never reach here. But apparently the compiler isn't smart enough to realize this.
      }

      /*
      // Below is an example of what a Visitor should look like:
      struct Visitor
      {
         const types_manager_common& tm;
         state_t state;

         Visitor(const types_manager_common& tm, const state_t& state) : tm(tm), state(state) {}

         using traversal_shortcut = types_manager_common::traversal_shortcut;
         using struct_type   = types_manager_common::struct_type;
         using array_type    = types_manager_common::array_type;
         using vector_type   = types_manager_common::vector_type;
         using optional_type = types_manager_common::optional_type;
         using variant_type  = types_manager_common::variant_type; 

         template<typename T>
         traversal_shortcut operator()(const T&) { return types_manager_common::no_shortcut; } // Default implementation
         template<typename T, typename U>
         traversal_shortcut operator()(const T&, U) { return types_manager_common::no_shortcut; } // Default implementation

         traversal_shortcut operator()(type_id::builtin b); // Builtin
         traversal_shortcut operator()(struct_type t); // Struct. It is the responsibility of this function if it wishes to traverse over any of its field types.
         traversal_shortcut operator()(variant_type t); // Variant, but its individual cases will be traversed afterwards unless this function returns no_deeper or return_now.
         traversal_shortcut operator()(variant_type t, uint32_t case_index); // Called just before each variant case is processed.
         traversal_shortcut operator()(variant_type t, bool visited_all_cases); // Called after all cases of a particular variant have been processed.
         traversal_shortcut operator()(array_type t); // Array (small or general), but its element type will be traversed afterwards unless this function returns no_deeper or return_now.
         traversal_shortcut operator()(vector_type t); // Vector, but its element type will be traversed afterwards unless this function returns no_deeper or return_now.
         traversal_shortcut operator()(optional_type t); // Optional, but its element type will be traversed afterwards unless this function returns no_deeper or return_now.
         traversal_shortcut operator()(); // Void
      };
      */
   
   protected:
      
      const vector<uint32_t>&       types;
      const vector<field_metadata>& members;

      tuple<uint32_t, uint16_t, uint16_t> get_members_common(type_id::index_t struct_index)const;
      type_id::size_align                 get_size_align(type_id tid, uint32_t* cache_ptr)const;

   };

} }

