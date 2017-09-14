#include <eos/types/serialization_region.hpp>
#include <eos/types/abi_constructor.hpp>
#include <eos/types/types_constructor.hpp>
#include <eos/types/reflect.hpp>

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
   vector<type1> v;
};

struct type2 : public type1
{
   int16_t d;
   array<type1, 2> arr;
   string name;
};

struct type3
{
   uint64_t x;
   uint32_t y;
};

EOS_TYPES_REFLECT_STRUCT( type1,       (a)(b)(c)(v),  ((c, asc))((a, desc)) )
//                        struct name, struct fields, sequence of pairs for sorted fields (field name, asc/desc) where asc = ascending order and desc = descending order.      

EOS_TYPES_REFLECT_STRUCT_DERIVED( type2,       type1,    (d)(arr)(name), ((d, desc))((type1, asc)) )
//                                struct name, base name, struct fields, sequence of pairs for sorted members (as described above), however notice the base type name can also be included as member

EOS_TYPES_REFLECT_STRUCT( type3, (x)(y), ((x, asc))((y, asc)) )

EOS_TYPES_CREATE_TABLE( type1,  ((uint32_t, nu_asc, ({0}) ))((type3,    u_desc,     ({1,0})   )) )
//                      object, sequence of index tuples:   ((key_type, index_type, (mapping) )) where: mapping is a list (in curly brackets) of uint16_t member indices 
//                                                                                                        which map the key_type's sort members (specified in that order) to the object type's members,
//                                                                                               and    index_type = { u_asc,  // unique index sorted according to key_type in ascending order
//                                                                                                                     u_desc, // unique index sorted according to key_type in descending order
//                                                                                                                     nu_asc, // non-unique index sorted according to key_type in ascending order
//                                                                                                                     nu_desc // non-unique index sorted according to key_type in descending order
//                                                                                                                   }    

EOS_TYPES_REGISTER_TYPES( (type1), (type2) )
//                        tables,  other structs
// By registering the table for type1, it automatically registers type1 (since it is the object type of the table) and type3 (since it is the key type of one of the indices of the table).
// However, type2 is not discovered from the table of type1, so it needs to be explicitly registered through specifying it within the other structs arguments to the macro.

int main()
{
   using namespace eos::types;
   using std::cout;
   using std::endl;


   auto ac = initialize_types();
  
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

   auto type1_tid = type_id::make_struct(tc.get_struct_index<type1>());
   print_type_info(type1_tid);
   cout << endl;

   auto type2_tid = type_id::make_struct(tc.get_struct_index<type2>());
   print_type_info(type2_tid);
   cout << endl;

   auto type3_tid = type_id::make_struct(tc.get_struct_index<type3>());
   print_type_info(type3_tid);
   cout << endl;

   //auto tm = tc.destructively_extract_types_manager();
   auto tm = tc.copy_types_manager();
   serialization_region r(tm);

   type1 s1{ .a = 8, .b = 9, .c = {1, 2, 3, 4, 5, 6} };
   
   type2 s2;
   s2.a = 42; s2.b = 43; s2.d = -1; 
   s2.arr[0] = s1; 
   s2.arr[1].a = 1; s2.arr[1].b = 2; s2.arr[1].c.push_back(1); s2.arr[1].c.push_back(2);
   s2.name = "Hello, World!";

   r.write_type(s1, type1_tid);
      
   cout << "Raw data of serialization of s1:" << std::hex << endl;
   for( auto b : r.get_raw_data() )
      cout << (int) b << " ";
   cout << std::dec << endl;

   std::ostream_iterator<uint32_t> oi(cout, ", ");
   cout << "Reading back struct from raw data." << endl;
   type1 s3;
   r.read_type(s3, type1_tid);
   cout << "a = " << s3.a << ", b = " << s3.b << ", and c = [";
   std::copy(s3.c.begin(), s3.c.end(), oi);
   cout << "]" << endl << endl;


   raw_region r2;
   r2.extend(4);
   auto tbl_indx1 = tm.get_table_index(tm.get_table("type1"), 0);
   auto tbl_indx2 = tm.get_table_index(tm.get_table("type1"), 1);

   r2.set<uint32_t>(0, 13);
   auto c1 = tm.compare_object_with_key( r.get_raw_region(), r2, tbl_indx1 );
   cout << "Comparing object s1 with uint32_t key of value " << r2.get<uint32_t>(0) << " via the first index of table 'type1' gives: " << (int) c1 << endl;
   r2.set<uint32_t>(0, 5);
   auto c2 = tm.compare_object_with_key( r.get_raw_region(), r2, tbl_indx1 );
   cout << "Comparing object s1 with uint32_t key of value " << r2.get<uint32_t>(0) << " via the first index of table 'type1' gives: " << (int) c2 << endl;
   r2.set<uint32_t>(0, 8);
   auto c3 = tm.compare_object_with_key( r.get_raw_region(), r2, tbl_indx1 );
   cout << "Comparing object s1 with uint32_t key of value " << r2.get<uint32_t>(0) << " via the first index of table 'type1' gives: " << (int) c3 << endl;
   r2.clear();
   cout << endl;

   type3 s5{ .x = 8, .y = 9 };
   cout << "Struct s5 is {x = " << s5.x << ", y = " << s5.y << "}." << endl;
   serialization_region r3(tm);
   r3.write_type(s5, type3_tid);
   auto c4 = tm.compare_object_with_key( r.get_raw_region(), r3.get_raw_region(), tbl_indx2 );
   cout << "The relevant key extracted from object s1 would " << (c4 == 0 ? "be equal to" : (c4 < 0 ? "come before" : "come after")) 
        << " the key s5 according to the sorting specification of the second index of table 'type1'." << endl;
   cout << endl;


   r.clear();
   r.write_type(s2, type2_tid);

   cout << "Raw data of serialization of s2:" << std::hex << endl;
   for( auto b : r.get_raw_data() )
      cout << (int) b << " ";
   cout << std::dec << endl;

   cout << "Reading back struct from raw data." << endl;
   type2 s4;
   r.read_type(s4, type2_tid);
   cout << "a = " << s4.a << ", b = " << s4.b << ", c = [";
   std::copy(s4.c.begin(), s4.c.end(), oi);
   cout << "]";
   cout << ", d = " << s4.d << ", arr = [";
   for( auto i = 0; i < 2; ++i )
   {
      cout << "{a = " << s4.arr[i].a << ", b = " << s4.arr[i].b << ", and c = [";
      std::copy(s4.arr[i].c.begin(), s4.arr[i].c.end(), oi);
      cout << "]}, ";
   }
   cout << "], and name = '" << s4.name << "'" << endl << endl;

   auto type1_table = tm.get_table("type1");
   for( auto i = 0; i < tm.get_num_indices_in_table(type1_table); ++i )
   {
      auto ti = tm.get_table_index(type1_table, i);
      
      auto key_type = ti.get_key_type();
      string key_type_name;
      if( key_type.get_type_class() == type_id::builtin_type )
         key_type_name = type_id::get_builtin_type_name(key_type.get_builtin_type());
      else
         key_type_name = tc.get_struct_name(key_type.get_type_index());

      cout << "Index " << (i+1)  << " of the 'type1' table is " << (ti.is_unique() ? "unique" : "non-unique")
           << " and sorted according to the key type '" << key_type_name
           << "' in " << (ti.is_ascending() ? "ascending" : "descending" ) << " order." << endl;
   }

   return 0;
}