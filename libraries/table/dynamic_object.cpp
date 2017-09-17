#include <eos/table/dynamic_object.hpp>

namespace eos { namespace table {

   bool dynamic_object_compare::operator()(const dynamic_object& lhs, const dynamic_object& rhs)const
   {
      const auto& tm = ti.get_types_manager();
      auto c = tm.compare_objects(lhs.id, lhs.data, rhs.id, rhs.data, ti); 
      return (c < 0);
   }

   bool dynamic_key_compare::operator()(const dynamic_object& lhs, const dynamic_key& rhs)const
   {
      const auto& tm = ti.get_types_manager();
      auto c = tm.compare_object_with_key(lhs.data, rhs.data, ti); 
      return (c < 0);
   }

   bool dynamic_key_compare::operator()(const dynamic_key& lhs, const dynamic_object& rhs)const
   {
      const auto& tm = ti.get_types_manager();
      auto c = tm.compare_object_with_key(rhs.data, lhs.data, ti); 
      return (c > 0);
   }


} }

