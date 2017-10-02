#include <eos/eoslib/serialization_region.hpp>
#include <eos/types/abi_constructor.hpp>
#include <eos/types/abi.hpp>
#include <eos/types/types_constructor.hpp>
#include <eos/types/types_manager.hpp>
#include <eos/eoslib/full_types_manager.hpp>
#include <eos/types/reflect.hpp>
#include <eos/eoslib/type_traits.hpp>

#include <iostream>
#include <iterator>
#include <string>

using std::vector;
using std::array;
using std::string;

using eoslib::enable_if;

using eos::types::Vector;
using eos::types::Array;

template<typename T>
inline
typename enable_if<eos::types::reflector<T>::is_struct::value, eos::types::type_id::index_t>::type
get_struct_index(const eos::types::full_types_manager& ftm)
{
   return ftm.get_struct_index(eos::types::reflector<T>::name());
}

struct type1
{
   uint32_t a;
   uint64_t b;
   Vector<uint8_t> c;
   Vector<type1> v;
};

struct type2 : public type1
{
   int16_t d;
   Array<type1, 2> arr;
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


struct reflection_test1_types;
EOS_TYPES_REGISTER_TYPES( reflection_test1_types, (type1), (type2) )
//                        name,                   tables,  other structs
// By registering the table for type1, it automatically registers type1 (since it is the object type of the table) and type3 (since it is the key type of one of the indices of the table).
// However, type2 is not discovered from the table of type1, so it needs to be explicitly registered through specifying it within the other structs arguments to the macro.


struct abi_reflection;
EOS_TYPES_REGISTER_TYPES( abi_reflection, BOOST_PP_SEQ_NIL, (eos::types::ABI) )

int main()
{
   using namespace eos::types;
   using std::cout;
   using std::endl;

   auto abi_ac = types_initializer<abi_reflection>::init();
   types_constructor abi_tc(abi_ac.get_abi());
   auto abi_types_managers = abi_tc.destructively_extract_types_managers();
   const auto& abi_tm = abi_types_managers.second;
   auto abi_tid = type_id::make_struct(get_struct_index<ABI>(abi_tm));
   serialization_region abi_serializer(abi_tm);

   vector<type_id::index_t> abi_structs = { abi_tid.get_type_index(),
                                            get_struct_index<ABI::type_definition>(abi_tm),
                                            get_struct_index<ABI::struct_t>(abi_tm),
                                            get_struct_index<ABI::table_index>(abi_tm),
                                            get_struct_index<ABI::table>(abi_tm)
                                          };
   for( auto indx : abi_structs )
   {
      abi_tm.print_type(cout, type_id::make_struct(indx));
      cout << "Members:" << endl;
      for( auto f : abi_tm.get_all_members(indx) )
         cout << f << endl;
      cout << endl;
   }

   auto ac = types_initializer<reflection_test1_types>::init();
   abi_serializer.write_type(ac.get_abi(), abi_tid);

   cout << "Raw data of serialization of abi:" << endl;
   cout << abi_serializer.get_raw_region() << endl;
   
   ABI abi;
   abi_serializer.read_type(abi, abi_tid);

   types_constructor tc(abi);
   auto types_managers = tc.destructively_extract_types_managers();
   const auto& tm  = types_managers.first;
   const auto& ftm = types_managers.second;
   serialization_region r(ftm);

   auto print_type_info = [&](type_id tid)
   {
      auto index = tid.get_type_index();
      cout << ftm.get_struct_name(index) << " (index = " << index << "):" << endl;
      auto sa = ftm.get_size_align(tid);

      ftm.print_type(cout, tid);
      cout << "Size: " << sa.get_size() << endl;
      cout << "Alignment: " << (int) sa.get_align() << endl;

      cout << "Sorted members (in sort priority order):" << endl;
      for( auto f : ftm.get_sorted_members(index) )
         cout << f << endl;
   };

   auto type1_tid = type_id::make_struct(get_struct_index<type1>(ftm));
   print_type_info(type1_tid);
   cout << endl;

   auto type2_tid = type_id::make_struct(get_struct_index<type2>(ftm));
   print_type_info(type2_tid);
   cout << endl;

   auto type3_tid = type_id::make_struct(get_struct_index<type3>(ftm));
   print_type_info(type3_tid);
   cout << endl;

   type1 s1{ .a = 8, .b = 9, .c = {1, 2, 3, 4, 5, 6} };
   
   type2 s2;
   s2.a = 42; s2.b = 43; s2.d = -1; 
   s2.arr[0] = s1; 
   s2.arr[1].a = 1; s2.arr[1].b = 2; s2.arr[1].c.push_back(1); s2.arr[1].c.push_back(2);
   s2.name = "Hello, World!";

   r.write_type(s1, type1_tid);
      
   cout << "Raw data of serialization of s1:" << endl << r.get_raw_region();

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
   serialization_region r3(ftm);
   r3.write_type(s5, type3_tid);
   auto c4 = tm.compare_object_with_key( r.get_raw_region(), r3.get_raw_region(), tbl_indx2 );
   cout << "The relevant key extracted from object s1 would " << (c4 == 0 ? "be equal to" : (c4 < 0 ? "come before" : "come after")) 
        << " the key s5 according to the sorting specification of the second index of table 'type1'." << endl;
   cout << endl;


   r.clear();
   r.write_type(s2, type2_tid);

   cout << "Raw data of serialization of s2:" << endl << r.get_raw_region();

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
         key_type_name = ftm.get_struct_name(key_type.get_type_index());

      cout << "Index " << (i+1)  << " of the 'type1' table is " << (ti.is_unique() ? "unique" : "non-unique")
           << " and sorted according to the key type '" << key_type_name
           << "' in " << (ti.is_ascending() ? "ascending" : "descending" ) << " order." << endl;
   }

   return 0;
}
