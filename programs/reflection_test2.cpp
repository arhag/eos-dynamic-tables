#include <eos/types/serialization_region.hpp>
#include <eos/types/abi_constructor.hpp>
#include <eos/types/types_constructor.hpp>
#include <eos/types/reflect.hpp>

#include <type_traits>
#include <iostream>
#include <iterator>
#include <string>

using std::vector;
using std::array;
using std::string;

struct eos_symbol;
struct currency_symbol;

template<typename NumberType, class TokenType>
class token
{
private:

   using enable_if_t = typename std::enable_if< std::is_integral<NumberType>::value && !std::is_same<NumberType, bool>::value >::type; 
      
   NumberType quantity;

public:

   token()
      : quantity()
   {}

   // This constructor taking NumberType and the conversion method to NumberType below it allow this custom class to be used 
   // as if it is the primitive integer type NumberType by the serialization/deserialization system.
 
   explicit token( NumberType v )
      : quantity(v)
   {}

   explicit operator NumberType()const { return quantity; }

   token& operator-=( const token& a ) {
    assert( quantity >= a.quantity ); // "integer underflow subtracting token balance" );
    quantity -= a.quantity;
    return *this;
   }

   token& operator+=( const token& a ) {
    assert( quantity + a.quantity >= a.quantity ); //  "integer overflow adding token balance" );
    quantity += a.quantity;
    return *this;
   }

   inline friend token operator+( const token& a, const token& b ) {
    token result = a;
    result += b;
    return result;
   }

   inline friend token operator-( const token& a, const token& b ) {
    token result = a;
    result -= b;
    return result;
   }

   friend bool operator <= ( const token& a, const token& b ) { return a.quantity <= b.quantity; }
   friend bool operator <  ( const token& a, const token& b ) { return a.quantity <  b.quantity; }
   friend bool operator >= ( const token& a, const token& b ) { return a.quantity >= b.quantity; }
   friend bool operator >  ( const token& a, const token& b ) { return a.quantity >  b.quantity; }
   friend bool operator == ( const token& a, const token& b ) { return a.quantity == b.quantity; }
   friend bool operator != ( const token& a, const token& b ) { return a.quantity != b.quantity; }

   explicit operator bool()const { return quantity != 0; }
};

using eos_token      = token<uint64_t, eos_symbol>;
using currency_token = token<uint64_t, currency_symbol>; // Compilation error if you try to add a currency_token to an eos_token

EOS_TYPES_REFLECT_BUILTIN( eos_token, builtin_uint64 )
EOS_TYPES_REFLECT_BUILTIN( currency_token, builtin_uint64 )
// Both eos_token and currency_token will be treated as a uint64_t in the type system.

class time_point_sec
{
public:
   time_point_sec()
      :utc_seconds(0){}

   explicit time_point_sec(uint32_t seconds )
      :utc_seconds(seconds){}

   static time_point_sec maximum() { return time_point_sec(0xffffffff); }
   static time_point_sec min() { return time_point_sec(0); }

   uint32_t sec_since_epoch()const { return utc_seconds; }

   friend bool      operator < ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds < b.utc_seconds; }
   friend bool      operator > ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds > b.utc_seconds; }
   friend bool      operator <= ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds <= b.utc_seconds; }
   friend bool      operator >= ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds >= b.utc_seconds; }
   friend bool      operator == ( const time_point_sec& a, const time_point_sec& b ) { return a.utc_seconds == b.utc_seconds; }
   friend bool      operator != ( const time_point_sec& a, const time_point_sec& b ) { return a.utc_seconds != b.utc_seconds; }
   time_point_sec&  operator += ( uint32_t m ) { utc_seconds+=m; return *this; }
   time_point_sec&  operator -= ( uint32_t m ) { utc_seconds-=m; return *this; }
   time_point_sec   operator +( uint32_t offset )const { return time_point_sec(utc_seconds + offset); }
   time_point_sec   operator -( uint32_t offset )const { return time_point_sec(utc_seconds - offset); }

   friend class eos::types::serialization_region;
   friend class eos::types::deserialize_visitor;

private:
   uint32_t utc_seconds;

   // The conversion method to the integer type (and the constructor taking the integer type) can also be private if desired 
   // as long as the appropriate classes are added as friends (see above).
   // In this case the constructor was kept public because it was useful for other purposes.
   // But the conversion was kept private since users of this class are expected to use the more meaningful `sec_since_epoch` method.
   explicit operator uint32_t()const { return utc_seconds; }
};

EOS_TYPES_REFLECT_BUILTIN( time_point_sec, builtin_uint32 )


struct order_id
{
   string name; // EOS.IO account names are actually represented with a uint64_t, but this is just to show off the String type in this new type system.
   uint32_t id;
};

EOS_TYPES_REFLECT_STRUCT( order_id, (name)(id), ((name, asc))((id, asc)) )

using eos::types::rational;

struct bid
{
   order_id       buyer;
   rational       price;
   eos_token      quantity;
   time_point_sec expiration;
};

EOS_TYPES_REFLECT_STRUCT( bid, (buyer)(price)(quantity)(expiration) )

struct ask
{
   order_id       seller;
   rational       price;
   currency_token quantity;
   time_point_sec expiration;
};

EOS_TYPES_REFLECT_STRUCT( ask, (seller)(price)(quantity)(expiration) )

struct order_id_key
{
   order_id     oid;
};

EOS_TYPES_REFLECT_STRUCT( order_id_key, (oid), ((oid, asc)) )

struct expiration_key
{
   time_point_sec expiration;
   order_id       oid;
};

EOS_TYPES_REFLECT_STRUCT( expiration_key, (expiration)(oid), ((expiration, asc))((oid, asc)) )

EOS_TYPES_CREATE_TABLE( bid,  
                        (( order_id_key,   u_asc,   ({0})    ))
                        (( rational,       nu_desc, ({1})    ))
                        (( expiration_key, u_asc,   ({3, 0}) ))
                      )

EOS_TYPES_CREATE_TABLE( ask,  
                        (( order_id_key,   u_asc,   ({0})    ))
                        (( rational,       nu_asc,  ({1})    ))
                        (( expiration_key, u_asc,   ({3, 0}) ))
                      )

struct reflection_test2_types;
EOS_TYPES_REGISTER_TYPES( reflection_test2_types, (bid)(ask) )

int main()
{
   using namespace eos::types;
   using std::cout;
   using std::endl;


   auto ac = types_initializer<reflection_test2_types>::init();
  
   types_constructor tc(ac.get_abi());

   auto print_type_info = [&](type_id tid)
   {
      auto index = tid.get_type_index();
      cout << tc.get_struct_name(index) << " (index = " << index << "):" << endl;
      auto sa = tc.get_size_align_of_struct(index);

      tc.print_type(cout, tid);
      cout << "Size: " << sa.get_size() << endl;
      cout << "Alignment: " << (int) sa.get_align() << endl;

      cout << "Sorted members (in sort priority order):" << endl;
      for( auto f : tc.get_sorted_members(index) )
         cout << f << endl;
   };

   auto order_id_tid = type_id::make_struct(tc.get_struct_index<order_id>());
   print_type_info(order_id_tid);
   cout << endl;

   auto bid_tid = type_id::make_struct(tc.get_struct_index<bid>());
   print_type_info(bid_tid);
   cout << endl;

   auto ask_tid = type_id::make_struct(tc.get_struct_index<ask>());
   print_type_info(ask_tid);
   cout << endl;

   //auto tm = tc.destructively_extract_types_manager();
   auto tm = tc.copy_types_manager();
   serialization_region r(tm);

   auto tbl_indx1 = tm.get_table_index(tm.get_table("bid"), 0);
   auto tbl_indx2 = tm.get_table_index(tm.get_table("bid"), 1);
   auto tbl_indx3 = tm.get_table_index(tm.get_table("bid"), 2);

   time_point_sec exp_time(1506000000);

   ask a1 = { .seller = { .name = "Alice", .id = 0 },
              .price = { .numerator = 8 , .denominator = 5 },
              .quantity = currency_token(10),
              .expiration = exp_time
            }; 

   bid b1 = { .buyer = { .name = "Bob", .id = 0 }, 
              .price = { .numerator = 6 , .denominator = 7 },
              .quantity = eos_token(4),
              .expiration = exp_time
            }; 

   bid b2 = { .buyer = { .name = "Bob", .id = 1 }, 
              .price = { .numerator = 1 , .denominator = 1 },
              .quantity = (eos_token(7) - eos_token(4)), // Note that (eos_token(7) - currency_token(4)) would be a compilation error.
              .expiration = exp_time + 6
            }; 

   r.write_type( a1, ask_tid );
 
   cout << "Raw data of serialization of a1:" << endl << r.get_raw_region();

   raw_region a1_data = r.get_raw_region();  

   r.clear();   
   r.write_type( b1, bid_tid );
 
   cout << "Raw data of serialization of b1:" << endl << r.get_raw_region();

   raw_region b1_data = r.get_raw_region();  

   r.clear();   
   r.write_type( b2, bid_tid );
 
   cout << "Raw data of serialization of b2:" << endl << r.get_raw_region();
   cout << endl;

   raw_region b2_data = r.get_raw_region();  

   bid b2_copy;
   r.read_type( b2_copy, bid_tid );
   if( b2_copy.buyer.name != b2.buyer.name || b2_copy.quantity != b2.quantity || b2_copy.expiration != b2.expiration )
      cout << "Something went wrong with deserialization of b2." << endl;

   r.clear();
   r.write_type( rational(13, 14), type_id(type_id::builtin_rational) );

   cout << "Raw data of serialization of rat_key:" << endl << r.get_raw_region();

   raw_region rat_key = r.get_raw_region();


   auto c1 = tm.compare_object_with_key( b1_data, rat_key, tbl_indx2 );
   cout << "Comparing b1 by price with rat_key: " << (int) c1 << endl;
   auto c2 = tm.compare_object_with_key( b2_data, rat_key, tbl_indx2 );
   cout << "Comparing b2 by price with rat_key: " << (int) c2 << endl;
   cout << endl;

   auto c3 = tm.compare_objects( 0, b1_data, 1, b2_data, tbl_indx2 );
   cout << "b1 " << (c3 == 0 ? "is equal to" : (c3 < 0 ? "comes before" : "comes after")) 
        << " b2 by price." << endl;
   auto c4 = tm.compare_objects( 0, b1_data, 1, b2_data, tbl_indx1 );
   cout << "b1 " << (c4 == 0 ? "is equal to" : (c4 < 0 ? "comes before" : "comes after")) 
        << " b2 by order_id." << endl;
   auto c5 = tm.compare_objects( 0, b1_data, 1, b2_data, tbl_indx3 );
   cout << "b1 " << (c5 == 0 ? "is equal to" : (c5 < 0 ? "comes before" : "comes after")) 
        << " b2 by expiration." << endl;
   cout << endl;

   auto bid_table = tm.get_table("bid");
   for( auto i = 0; i < tm.get_num_indices_in_table(bid_table); ++i )
   {
      auto ti = tm.get_table_index(bid_table, i);
      
      auto key_type = ti.get_key_type();
      string key_type_name;
      if( key_type.get_type_class() == type_id::builtin_type )
         key_type_name = type_id::get_builtin_type_name(key_type.get_builtin_type());
      else
         key_type_name = tc.get_struct_name(key_type.get_type_index());

      cout << "Index " << (i+1)  << " of the 'bid' table is " << (ti.is_unique() ? "unique" : "non-unique")
           << " and sorted according to the key type '" << key_type_name
           << "' in " << (ti.is_ascending() ? "ascending" : "descending" ) << " order." << endl;
   }

   return 0;
}
