#include <eos/types/serialization_region.hpp>
#include <eos/types/abi_constructor.hpp>
#include <eos/types/types_constructor.hpp>
#include <eos/types/reflect.hpp>
#include <eos/table/dynamic_table.hpp>

#include <iostream>
#include <iterator>
#include <string>

using std::vector;
using std::array;
using std::string;

struct type1
{
   uint32_t a;
   uint64_t b;
   vector<uint8_t> c;
};

struct type2
{
   uint64_t x;
   uint32_t y;
};

EOS_TYPES_REFLECT_STRUCT( type1, (a)(b)(c), ((c, asc))((a, desc)) )

EOS_TYPES_REFLECT_STRUCT( type2, (x)(y), ((x, asc))((y, asc)) )

EOS_TYPES_CREATE_TABLE( type1,  
                        (( uint32_t,        nu_asc, ({0})   ))
                        (( type2,           u_desc, ({1,0}) )) 
                        (( vector<uint8_t>, nu_asc, ({2})   )) 
                      )

struct table_test1_types;
EOS_TYPES_REGISTER_TYPES( table_test1_types, (type1) )

int main()
{
   using namespace eos::types;
   using namespace eos::table;
   using std::cout;
   using std::endl;


   auto ac = types_initializer<table_test1_types>::init();
  
   types_constructor tc(ac.get_abi());

   auto print_type_info = [&](type_id tid)
   {
      auto index = tid.get_type_index();
      cout << tc.get_struct_name(index) << ":" << endl;
      auto sa = tc.get_size_align_of_struct(index);

      tc.print_type(cout, tid);
      cout << "Size: " << sa.get_size() << endl;
      cout << "Alignment: " << (int) sa.get_align() << endl;

      cout << "Sorted members (in sort priority order):" << endl;
      for( auto f : tc.get_sorted_members(index) )
         cout << f << endl;
   };

   auto print_raw_data = [&](const raw_region& r, const char* name)
   {
      cout << "Raw data of serialization of " << name << ":" << endl;
      cout << r << endl; 
   };

   auto type1_tid = type_id::make_struct(tc.get_struct_index<type1>());
   print_type_info(type1_tid);
   cout << endl;

   auto type2_tid = type_id::make_struct(tc.get_struct_index<type2>());
   print_type_info(type2_tid);
   cout << endl;

   auto tm = tc.destructively_extract_types_manager();
   //auto tm = tc.copy_types_manager();

   dynamic_table_3 table_type1(make_dynamic_table_ctor_args_list<3>(tm, tm.get_table("type1")));

   serialization_region r(tm);

   cout << "Size of table 'type1': " << table_type1.size() << endl << endl;

   type1 s1{ .a = 8, .b = 4, .c = {1, 2, 3, 4, 5, 6} };
   dynamic_object s1_obj{.id = 0};
   r.write_type(s1, type1_tid);
   print_raw_data(r.get_raw_region(), "s1");
   s1_obj.data = r.move_raw_region();

   cout << "Inserting object s1 into table 'type1'... "; 
   auto res1 = table_type1.insert(s1_obj);
   cout << (res1.second ? "Success." : "Failed.") << endl;

   cout << "Size of table 'type1': " << table_type1.size() << endl << endl;

   type1 s2{ .a = 8, .b = 10, .c = {13} }; // Insertion into table would fail (i.e. res2.second == false) if b was 4 because of uniqueness violation of index 2 (with type2 key).
   dynamic_object s2_obj{.id = 1};
   r.write_type(s2, type1_tid);
   print_raw_data(r.get_raw_region(), "s2");
   s2_obj.data = r.move_raw_region();

   cout << "Inserting object s2 into table 'type1'... "; 
   auto res2 = table_type1.insert(s2_obj);
   cout << (res2.second ? "Success." : "Failed.") << endl;

   cout << "Size of table 'type1': " << table_type1.size() << endl << endl;

   type1 s3{ .a = 3, .b = 4, .c = {13, 14} };
   dynamic_object s3_obj{.id = 2};
   r.write_type(s3, type1_tid);
   print_raw_data(r.get_raw_region(), "s3");
   s3_obj.data = r.move_raw_region();

   cout << "Inserting object s3 into table 'type1'... "; 
   auto res3 = table_type1.insert(s3_obj);
   cout << (res3.second ? "Success." : "Failed.") << endl;

   cout << "Size of table 'type1': " << table_type1.size() << endl << endl;

   cout << "Table 'type1' objects sorted by index 1 (field 'a' of type1 as the key sorted in ascending order):" << endl;
   for( const auto& obj : table_type1.get<1>() )
   {
      cout << "Object id = " << obj.id << " and data = " << endl << obj.data << endl;
   }   
   cout << endl;

   cout << "Table 'type1' objects sorted by index 2 (type2, mapped to fields 'b' and 'a' of type1, as the key sorted by type2's sorting specification except in descending order):" << endl;
   for( const auto& obj : table_type1.get<2>() )
   {
      cout << "Object id = " << obj.id << " and data = " << endl << obj.data << endl;
   }   
   cout << endl;

   cout << "Table 'type1' objects sorted by index 3 (field 'c' of type1 as the key sorted in ascending lexicographical order):" << endl;
   for( const auto& obj : table_type1.get<3>() )
   {
      cout << "Object id = " << obj.id << " and data = " << endl << obj.data << endl;
   }   
   cout << endl;


   dynamic_key_compare dkc1(tm.get_table_index(tm.get_table("type1"), 0)); // Comparison functor for lookups in index 1 of table 'type1'

   uint32_t v = 3;
   r.write_type(v, type_id(type_id::builtin_uint32));
   dynamic_key k1(r.get_raw_region());

   auto itr = table_type1.get<1>().lower_bound(k1, dkc1);
   if( itr != table_type1.get<1>().end() )
   {
      cout << "Found lower bound using index 1 of table 'type1' and a key of value " << v << ":" << endl;
      cout << "Object id = " << itr->id << " and data = " << endl << itr->data << endl;
   }

   itr = table_type1.get<1>().upper_bound(k1, dkc1);
   if( itr != table_type1.get<1>().end() )
   {
      cout << "Found upper bound using index 1 of table 'type1' and a key of value " << v << ":" << endl;
      cout << "Object id = " << itr->id << " and data = " << endl << itr->data << endl;
   }

   return 0;
}
