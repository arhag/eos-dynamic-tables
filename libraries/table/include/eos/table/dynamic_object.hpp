#pragma once

#include <eos/eoslib/raw_region.hpp>
#include <eos/types/types_manager.hpp>

namespace eos { namespace table {

   using namespace eos::types;

   struct dynamic_object
   {
      uint64_t   id = 0;
      raw_region data;
   };

   class dynamic_object_compare
   {
   public:

      dynamic_object_compare(const types_manager::table_index& ti)
         : ti(ti)
      {}

      bool operator()(const dynamic_object& lhs, const dynamic_object& rhs)const;

   private:
      types_manager::table_index ti;
   };

   struct dynamic_key
   {
      dynamic_key(const raw_region& r)
         : data(r) 
      {}

      dynamic_key(const dynamic_object& o)
         : data(o.data)
      {}

      const raw_region& data;
   };

   class dynamic_key_compare
   {
   public:

      dynamic_key_compare(const types_manager::table_index& ti)
         : ti(ti)
      {}

      bool operator()(const dynamic_object& lhs, const dynamic_key& rhs)const;
      bool operator()(const dynamic_key& lhs,    const dynamic_object& rhs)const;

   private:
      types_manager::table_index ti;
   };

} }

