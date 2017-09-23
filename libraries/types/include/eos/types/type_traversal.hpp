#pragma once

#include <eos/types/type_id.hpp>

namespace eos { namespace types {

namespace type_traversal {

   enum traversal_shortcut
   {
      no_shortcut,
      no_deeper,
      return_now
   };

   struct struct_type   { type_id::index_t index; };
   struct array_type    { type_id element_type; uint32_t num_elements; };
   struct vector_type   { type_id element_type;   };
   struct optional_type { type_id element_type; type_id optional_tid; };
   struct variant_type  { type_id::index_t index; };

   // Type traversal: (Caller needs to ensure types vector is in a coherent state, or else things will go horribly wrong)
   template<typename Visitor>
   traversal_shortcut traverse_type(type_id tid, Visitor& v, const vector<uint32_t>& types)
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
            traversal_shortcut s = traverse_type(element_type, v, types);
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
            traversal_shortcut s = traverse_type(element_type, v, types);
            return (s == no_deeper ? no_shortcut : s);
         } 
         case type_id::optional_struct_type:
         {
            auto element_type = tid.get_element_type();

            traversal_shortcut traverse_element = v(optional_type{element_type, tid});
            switch( traverse_element )
            {
               case return_now:
                  return return_now;
               case no_deeper:
                  return no_shortcut;
               case no_shortcut:
                  break;
            }
            traversal_shortcut s = traverse_type(element_type, v, types);
            return (s == no_deeper ? no_shortcut : s);
         }
         case type_id::variant_or_optional_type:
         {
            auto index = tid.get_type_index();
            if( types[index+1] >= type_id::variant_case_limit ) // If actually an optional
            {
               //type_id element_type(types[index+1], true);
               type_id element_type(types[index+1]);
               traversal_shortcut traverse_element = v(optional_type{element_type, tid});
               switch( traverse_element )
               {
                  case return_now:
                     return return_now;
                  case no_deeper:
                     return no_shortcut;
                  case no_shortcut:
                     break;
               }
               traversal_shortcut s = traverse_type(element_type, v, types);
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
                  s = traverse_type(t, v, types);
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
            traversal_shortcut s = traverse_type(element_type, v, types);
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
            traversal_shortcut s = traverse_type(element_type, v, types);
            return (s == no_deeper ? no_shortcut : s);
         }
      }
      return no_shortcut; // Should never reach here. But apparently the compiler isn't smart enough to realize this.
   }

   /*
   // Below is an example of what a Visitor should look like:
   struct Visitor
   {
      const types_manager& tm;
      state_t state;

      Visitor(const types_manager& tm, const state_t& state) : tm(tm), state(state) {}

      using traversal_shortcut = type_traversal::traversal_shortcut;
      using array_type    = type_traversal::array_type;
      using struct_type   = type_traversal::struct_type;
      using vector_type   = type_traversal::vector_type;
      using optional_type = type_traversal::optional_type;
      using variant_type  = type_traversal::variant_type; 

      template<typename T>
      traversal_shortcut operator()(const T&) { return type_traversal::no_shortcut; } // Default implementation
      template<typename T, typename U>
      traversal_shortcut operator()(const T&, U) { return type_traversal::no_shortcut; } // Default implementation

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

}

} }

