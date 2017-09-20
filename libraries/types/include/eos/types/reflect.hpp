#pragma once

#include <eos/types/raw_region.hpp>
#include <eos/types/abi_constructor.hpp>
#include <eos/types/type_id.hpp>

#include <type_traits>
#include <stdexcept>
#include <array>
#include <iterator>
#include <utility>
#include <vector>
#include <tuple>
#include <string>
#include <typeinfo>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/preprocessor/facilities/overload.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#ifndef _MSC_VER
  #define TEMPLATE template
#else
  // Disable warning C4482: nonstandard extention used: enum 'enum_type::enum_value' used in qualified name
  #pragma warning( disable: 4482 )
  #define TEMPLATE
#endif

#define EOS_TYPES_REFLECT_GET_MEMBER_TYPE(structure, member) decltype((static_cast<structure*>(nullptr))->member)

#define EOS_TYPES_REFLECT_VISIT_BASE(T, B, base) _v.TEMPLATE operator()<T, B>( base );

#define EOS_TYPES_REFLECT_VISIT_BASE_TYPE(T, B)  _v.TEMPLATE operator()<T, B>();

#define EOS_TYPES_REFLECT_VISIT_MEMBER( r, start_index, index, elem )                                        \
{ using member_type = EOS_TYPES_REFLECT_GET_MEMBER_TYPE(type, elem);                                         \
  _v.TEMPLATE operator()<member_type,type,&type::elem>( _s, BOOST_PP_STRINGIZE(elem), start_index + index ); \
}

#define EOS_TYPES_REFLECT_VISIT_MEMBER_TYPE( r, start_index, index, elem )                                   \
{ using member_type = EOS_TYPES_REFLECT_GET_MEMBER_TYPE(type, elem);                                         \
  _v.TEMPLATE operator()<member_type,type,&type::elem>( BOOST_PP_STRINGIZE(elem), start_index + index );     \
}

#define EOS_TYPES_REFLECT_VISIT_STRUCT_START(T)  if( !_v.TEMPLATE operator()<T>( BOOST_PP_STRINGIZE(T) ) ) { return; }


#define EOS_TYPES_REFLECT_VISIT_STRUCT_END(T)    _v.TEMPLATE operator()<T>();

#define EOS_TYPES_REFLECT_STRINGIZE(r, data, elem) BOOST_PP_STRINGIZE(elem)

#define EOS_TYPES_REFLECT_MEMBER_NAME(r, data, elem) BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2, 0, elem))

#define EOS_TYPES_REFLECT_SORT_asc true
#define EOS_TYPES_REFLECT_SORT_desc false

#define EOS_TYPES_REFLECT_SORT_ORDER(r, data, elem) BOOST_PP_CAT(EOS_TYPES_REFLECT_SORT_,BOOST_PP_TUPLE_ELEM(2, 1, elem))

#define EOS_TYPES_REFLECT_FIELD_NAMES(T, fields)                                                             \
const char* const eos::types::reflector<T>::field_names[BOOST_PP_SEQ_SIZE(fields)] = {                       \
      BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(EOS_TYPES_REFLECT_STRINGIZE, _, fields))                      \
};                                                                         

#define EOS_TYPES_REFLECT_SORT_MEMBER(T, member_sort)                                                        \
const char* const eos::types::reflector<T>::sorted_member_names[BOOST_PP_SEQ_SIZE(member_sort)] = {          \
   BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(EOS_TYPES_REFLECT_MEMBER_NAME, _, member_sort))                  \
};                                                                                                           \
bool const eos::types::reflector<T>::sorted_member_dir[BOOST_PP_SEQ_SIZE(member_sort)] = {                   \
   BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(EOS_TYPES_REFLECT_SORT_ORDER, _, member_sort))                   \
};

#define EOS_TYPES_REFLECT_INCREMENTER(r, op, elem ) op 1 

#define EOS_TYPES_REFLECT_MEMBER_COUNT(fields, member_sort)                                                  \
      enum  field_count_enum {                                                                               \
         field_count = 0 BOOST_PP_SEQ_FOR_EACH(EOS_TYPES_REFLECT_INCREMENTER, +, fields )                    \
      };                                                                                                     \
      enum sorted_member_count_enum {                                                                        \
         sorted_member_count = 0 BOOST_PP_SEQ_FOR_EACH(EOS_TYPES_REFLECT_INCREMENTER, +, member_sort )       \
      };

#define EOS_TYPES_REFLECT_GET_MEMBER_INFO_START                                                              \
      static inline void get_member_info( const char* * * field_names,                                       \
                                          const char* * * sorted_member_names,                               \
                                          bool        * * sorted_member_dir   )                              \
      {                                                                                                      

#define EOS_TYPES_REFLECT_GET_MEMBER_INFO_NOOP                                                               \
      EOS_TYPES_REFLECT_GET_MEMBER_INFO_START                                                                \
      }

#define EOS_TYPES_REFLECT_GET_MEMBER_INFO_COMMON(fields)                                                     \
      EOS_TYPES_REFLECT_GET_MEMBER_INFO_START                                                                \
         static const char* _field_names[BOOST_PP_SEQ_SIZE(fields)] = {                                      \
            BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(EOS_TYPES_REFLECT_STRINGIZE, _, fields))                \
         };                                                                                                  \
         *field_names = _field_names;                                                                        

#define EOS_TYPES_REFLECT_GET_MEMBER_INFO_FIELDS_ONLY(fields)                                                \
      EOS_TYPES_REFLECT_GET_MEMBER_INFO_COMMON(fields)                                                       \
      }

#define EOS_TYPES_REFLECT_GET_MEMBER_INFO(fields, member_sort)                                               \
      EOS_TYPES_REFLECT_GET_MEMBER_INFO_COMMON(fields)                                                       \
          static const char* _sorted_member_names[BOOST_PP_SEQ_SIZE(member_sort)] = {                        \
            BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(EOS_TYPES_REFLECT_MEMBER_NAME, _, member_sort))         \
         };                                                                                                  \
         *sorted_member_names = _sorted_member_names;                                                        \
         bool               _sorted_member_dir[BOOST_PP_SEQ_SIZE(member_sort)] = {                           \
            BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(EOS_TYPES_REFLECT_SORT_ORDER, _, member_sort))          \
         };                                                                                                  \
         *sorted_member_dir   = _sorted_member_dir;                                                          \
      }

#define EOS_TYPES_REFLECT_STRUCT_START(T)                                                                    \
namespace eos { namespace types {                                                                            \
   template <>                                                                                               \
   struct reflector<T>                                                                                       \
   {                                                                                                         \
      using is_defined = std::true_type;                                                                     \
      using is_struct  = std::true_type;                                                                     \
      using type       = T;                                                                                  

#define EOS_TYPES_REFLECT_STRUCT_END_HELPER(NAME)                                                            \
      static inline uint32_t register_struct(eos::types::abi_constructor& ac, const std::vector<type_id>& ft)\
      {                                                                                                      \
         const char* * field_names = nullptr;                                                                \
         const char* * sorted_member_names = nullptr;                                                        \
         bool        * sorted_member_dir = nullptr;                                                          \
         get_member_info(&field_names, &sorted_member_names, &sorted_member_dir);                            \
         return eos::types::register_struct(ac, ft, NAME,                                                    \
                                            field_names, static_cast<uint32_t>(field_count),                 \
                                            sorted_member_names, sorted_member_dir,                          \
                                            static_cast<uint32_t>(sorted_member_count));                     \
      }                                                                                                      \
      static inline const char* name()                                                                       \
      {                                                                                                      \
         return NAME;                                                                                        \
      }                                                                                                      \
  };                                                                                                         \
} }                                                                                                          

#define EOS_TYPES_REFLECT_STRUCT_END(T, fields)                                                              \
      template<typename Visitor>                                                                             \
      static void visit(Visitor& _v)                                                                         \
      {                                                                                                      \
         EOS_TYPES_REFLECT_VISIT_STRUCT_START(T)                                                             \
         BOOST_PP_SEQ_FOR_EACH_I(EOS_TYPES_REFLECT_VISIT_MEMBER_TYPE, 0, fields)                             \
         EOS_TYPES_REFLECT_VISIT_STRUCT_END(T)                                                               \
      }                                                                                                      \
      template<typename Visitor>                                                                             \
      static void visit(const type& _s, const Visitor& _v)                                                   \
      {                                                                                                      \
         BOOST_PP_SEQ_FOR_EACH_I(EOS_TYPES_REFLECT_VISIT_MEMBER, 0, fields)                                  \
      }                                                                                                      \
      template<typename Visitor>                                                                             \
      static void visit(type& _s, const Visitor& _v)                                                         \
      {                                                                                                      \
         BOOST_PP_SEQ_FOR_EACH_I(EOS_TYPES_REFLECT_VISIT_MEMBER, 0, fields)                                  \
      }                                                                                                      \
      EOS_TYPES_REFLECT_STRUCT_END_HELPER(BOOST_PP_STRINGIZE(T))

#define EOS_TYPES_REFLECT_STRUCT_1  

#define EOS_TYPES_REFLECT_STRUCT_2(T, fields)                                                                \
EOS_TYPES_REFLECT_STRUCT_START(T)                                                                            \
EOS_TYPES_REFLECT_MEMBER_COUNT(fields, BOOST_PP_SEQ_NIL)                                                     \
EOS_TYPES_REFLECT_GET_MEMBER_INFO_FIELDS_ONLY(fields)                                                        \
EOS_TYPES_REFLECT_STRUCT_END(T, fields)                                                 

#define EOS_TYPES_REFLECT_STRUCT_3(T, fields, member_sort)                                                   \
EOS_TYPES_REFLECT_STRUCT_START(T)                                                                            \
EOS_TYPES_REFLECT_MEMBER_COUNT(fields, member_sort)                                                          \
EOS_TYPES_REFLECT_GET_MEMBER_INFO(fields, member_sort)                                                       \
EOS_TYPES_REFLECT_STRUCT_END(T, fields)                         

#if !BOOST_PP_VARIADICS_MSVC
#define EOS_TYPES_REFLECT_STRUCT(...) \
   BOOST_PP_OVERLOAD(EOS_TYPES_REFLECT_STRUCT_,__VA_ARGS__)(__VA_ARGS__)
#else // for Visual C++
#define EOS_TYPES_REFLECT_STRUCT(...) \
   BOOST_PP_CAT(BOOST_PP_OVERLOAD(EOS_TYPES_REFLECT_STRUCT_,__VA_ARGS__)(__VA_ARGS__),BOOST_PP_EMPTY())
#endif

#define EOS_TYPES_REFLECT_STRUCT_DERIVED_START(T, B)                                                         \
namespace eos { namespace types {                                                                            \
   template <>                                                                                               \
   struct reflector<T>                                                                                       \
   {                                                                                                         \
      using is_defined = std::true_type;                                                                     \
      using is_struct  = std::true_type;                                                                     \
      using type       = T;                                                                                  

#define EOS_TYPES_REFLECT_STRUCT_DERIVED_END(T, B, fields)                                                   \
      template<typename Visitor>                                                                             \
      static void visit(Visitor& _v)                                                                         \
      {                                                                                                      \
         EOS_TYPES_REFLECT_VISIT_STRUCT_START(T)                                                             \
         EOS_TYPES_REFLECT_VISIT_BASE_TYPE(T, B)                                                             \
         BOOST_PP_SEQ_FOR_EACH_I(EOS_TYPES_REFLECT_VISIT_MEMBER_TYPE, 1, fields)                             \
         EOS_TYPES_REFLECT_VISIT_STRUCT_END(T)                                                               \
      }                                                                                                      \
      template<typename Visitor>                                                                             \
      static void visit(const type& _s, const Visitor& _v )                                                  \
      {                                                                                                      \
         EOS_TYPES_REFLECT_VISIT_BASE(T, B, static_cast<const B&>(_s))                                       \
         BOOST_PP_SEQ_FOR_EACH_I(EOS_TYPES_REFLECT_VISIT_MEMBER, 1, fields)                                  \
      }                                                                                                      \
      template<typename Visitor>                                                                             \
      static void visit(type& _s, const Visitor& _v )                                                        \
      {                                                                                                      \
         EOS_TYPES_REFLECT_VISIT_BASE(T, B, static_cast<B&>(_s))                                             \
         BOOST_PP_SEQ_FOR_EACH_I(EOS_TYPES_REFLECT_VISIT_MEMBER, 1, fields)                                  \
      }                                                                                                      \
      static inline uint32_t register_struct(eos::types::abi_constructor& ac, const std::vector<type_id>& ft)\
      {                                                                                                      \
         const char* * field_names = nullptr;                                                                \
         const char* * sorted_member_names = nullptr;                                                        \
         bool        * sorted_member_dir = nullptr;                                                          \
         get_member_info(&field_names, &sorted_member_names, &sorted_member_dir);                            \
         return eos::types::register_struct(ac, ft, BOOST_PP_STRINGIZE(T),                                   \
                                            field_names, static_cast<uint32_t>(field_count),                 \
                                            sorted_member_names, sorted_member_dir,                          \
                                            static_cast<uint32_t>(sorted_member_count),                      \
                                            BOOST_PP_STRINGIZE(B));                                          \
      }                                                                                                      \
      static inline const char* name()                                                                       \
      {                                                                                                      \
         return BOOST_PP_STRINGIZE(T);                                                                       \
      }                                                                                                      \
  };                                                                                                         \
} }                                                                                                          

#define EOS_TYPES_REFLECT_STRUCT_DERIVED_1 

#define EOS_TYPES_REFLECT_STRUCT_DERIVED_2(T, B)                                                             \
EOS_TYPES_REFLECT_STRUCT_DERIVED_START(T, B)                                                                 \
EOS_TYPES_REFLECT_MEMBER_COUNT(BOOST_PP_SEQ_NIL, BOOST_PP_SEQ_NIL)                                           \
EOS_TYPES_REFLECT_GET_MEMBER_INFO_NOOP()                                                                     \
EOS_TYPES_REFLECT_STRUCT_DERIVED_END(T, B, BOOST_PP_SEQ_NIL)                                                 \

#define EOS_TYPES_REFLECT_STRUCT_DERIVED_3(T, B, fields)                                                     \
EOS_TYPES_REFLECT_STRUCT_DERIVED_START(T, B)                                                                 \
EOS_TYPES_REFLECT_MEMBER_COUNT(fields, BOOST_PP_SEQ_NIL)                                                     \
EOS_TYPES_REFLECT_GET_MEMBER_INFO_FIELDS_ONLY(fields)                                                        \
EOS_TYPES_REFLECT_STRUCT_DERIVED_END(T, B, fields)                                                           \

#define EOS_TYPES_REFLECT_STRUCT_DERIVED_4(T, B, fields, member_sort)                                        \
EOS_TYPES_REFLECT_STRUCT_DERIVED_START(T, B)                                                                 \
EOS_TYPES_REFLECT_MEMBER_COUNT(fields, member_sort)                                                          \
EOS_TYPES_REFLECT_GET_MEMBER_INFO(fields, member_sort)                                                       \
EOS_TYPES_REFLECT_STRUCT_DERIVED_END(T, B, fields)                                                           \

#if !BOOST_PP_VARIADICS_MSVC
#define EOS_TYPES_REFLECT_STRUCT_DERIVED(...) \
   BOOST_PP_OVERLOAD(EOS_TYPES_REFLECT_STRUCT_DERIVED_,__VA_ARGS__)(__VA_ARGS__)
#else // for Visual C++
#define EOS_TYPES_REFLECT_STRUCT_DERIVED(...) \
   BOOST_PP_CAT(BOOST_PP_OVERLOAD(EOS_TYPES_REFLECT_STRUCT_DERIVED_,__VA_ARGS__)(__VA_ARGS__),BOOST_PP_EMPTY())
#endif

//#define EOS_TYPES_REFLECT_GET_TYPENAME(T) BOOST_PP_STRINGIZE(T)

#define EOS_TYPES_REFLECT_GET_TYPENAME(T) typeid(T).name()

#define EOS_TYPES_REFLECT_TEMPLATE_ARG(r, index, data) , data T##index

#define EOS_TYPES_REFLECT_VISIT_TUPLE_ELEMENT( r, index, data )                                              \
         _v.TEMPLATE operator()<data##index, type, index>( _s );

#define EOS_TYPES_REFLECT_VISIT_TUPLE_ELEMENT_TYPE( r, index, data )                                         \
         _v.TEMPLATE operator()<data##index, type, index>();

#define EOS_TYPES_REFLECT_PRODUCT_TYPE_HELPER(C, num_template_args, NAME, fields, member_sort)               \
namespace eos { namespace types {                                                                            \
   template<typename T0                                                                                      \
            BOOST_PP_REPEAT_FROM_TO(1, num_template_args, EOS_TYPES_REFLECT_TEMPLATE_ARG, typename)>         \
   struct reflector<C<T0                                                                                     \
                      BOOST_PP_REPEAT_FROM_TO(1, num_template_args, EOS_TYPES_REFLECT_TEMPLATE_ARG, )>>      \
   {                                                                                                         \
      using is_defined = std::true_type;                                                                     \
      using is_struct  = std::true_type;                                                                     \
      using type       = C<T0                                                                                \
                           BOOST_PP_REPEAT_FROM_TO(1, num_template_args, EOS_TYPES_REFLECT_TEMPLATE_ARG, )>; \
EOS_TYPES_REFLECT_MEMBER_COUNT(fields, member_sort)                                                          \
EOS_TYPES_REFLECT_GET_MEMBER_INFO(fields, member_sort)                                                       \
      template<typename Visitor>                                                                             \
      static void visit(Visitor& _v)                                                                         \
      {                                                                                                      \
         if( !_v.TEMPLATE operator()<type>( NAME ) ) { return; }                                             \
         BOOST_PP_REPEAT(num_template_args, EOS_TYPES_REFLECT_VISIT_TUPLE_ELEMENT_TYPE, T)                   \
         _v.TEMPLATE operator()<type>();                                                                     \
      }                                                                                                      \
      template<typename Visitor>                                                                             \
      static void visit(const type& _s, const Visitor& _v)                                                   \
      {                                                                                                      \
         BOOST_PP_REPEAT(num_template_args, EOS_TYPES_REFLECT_VISIT_TUPLE_ELEMENT, T)                        \
      }                                                                                                      \
      template<typename Visitor>                                                                             \
      static void visit(type& _s, const Visitor& _v)                                                         \
      {                                                                                                      \
         BOOST_PP_REPEAT(num_template_args, EOS_TYPES_REFLECT_VISIT_TUPLE_ELEMENT, T)                        \
      }                                                                                                      \
EOS_TYPES_REFLECT_STRUCT_END_HELPER(NAME)                         

#define EOS_TYPES_REFLECT_ANONYMOUS_FIELD(r, index, data) (data##index)

#define EOS_TYPES_REFLECT_ANONYMOUS_FIELD_ASCENDING(r, index, data) ((data##index, asc))

#if 0

#define EOS_TYPES_STRINGIZE_ALL_HELPER(...) #__VA_ARGS__

#define EOS_TYPES_STRINGIZE_ALL(...) EOS_TYPES_STRINGIZE_ALL_HELPER(__VA_ARGS__)

#define EOS_TYPES_REFLECT_PRODUCT_TYPE(C, Ts)                                                                \
EOS_TYPES_REFLECT_PRODUCT_TYPE_HELPER(C, BOOST_PP_SEQ_SIZE(Ts),                                              \
      EOS_TYPES_STRINGIZE_ALL(C<BOOST_PP_SEQ_ENUM(Ts)>),                                                     \
      BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(Ts), EOS_TYPES_REFLECT_ANONYMOUS_FIELD, f),                          \
      BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(Ts), EOS_TYPES_REFLECT_ANONYMOUS_FIELD_ASCENDING, f) )

#endif

#define EOS_TYPES_REFLECT_TUPLE(C, num_template_args)                                                        \
EOS_TYPES_REFLECT_PRODUCT_TYPE_HELPER(C, num_template_args,                                                  \
      EOS_TYPES_REFLECT_GET_TYPENAME(type),                                                                  \
      BOOST_PP_REPEAT(num_template_args, EOS_TYPES_REFLECT_ANONYMOUS_FIELD, f),                              \
      BOOST_PP_REPEAT(num_template_args, EOS_TYPES_REFLECT_ANONYMOUS_FIELD_ASCENDING, f) )


#define EOS_TYPES_REFLECT_SIMPLE_VISIT                         \
      template<typename Visitor>                               \
      static void visit(Visitor& _v)                           \
      {                                                        \
         _v.TEMPLATE operator()<type>();                       \
      }                                                        \
      template<typename Visitor>                               \
      static void visit(const type& _x, const Visitor& _v)     \
      {                                                        \
         _v.TEMPLATE operator()<type>(_x);                     \
      }                                                        \
      template<typename Visitor>                               \
      static void visit(type& _x, const Visitor& _v)           \
      {                                                        \
         _v.TEMPLATE operator()<type>(_x);                     \
      }                                                        \

#define EOS_TYPES_REFLECT_BUILTIN(T, B)                        \
namespace eos { namespace types {                              \
   template <>                                                 \
   struct reflector<T>                                         \
   {                                                           \
      using is_defined = std::true_type;                       \
      using is_struct  = std::false_type;                      \
      using is_builtin = std::true_type;                       \
      using type = T;                                          \
      static const type_id::builtin builtin_type = type_id::B; \
      EOS_TYPES_REFLECT_SIMPLE_VISIT                           \
   };                                                          \
} }

#define EOS_TYPES_REFLECT_ARRAY(C)                             \
namespace eos { namespace types {                              \
   template<typename T, size_t N>                              \
   struct reflector<C<T, N>>                                   \
   {                                                           \
      using is_defined = std::true_type;                       \
      using is_struct  = std::false_type;                      \
      using is_array   = std::true_type;                       \
      using type = C<T, N>;                                    \
      enum  num_elements_enum {                                \
         num_elements = N                                      \
      };                                                       \
      EOS_TYPES_REFLECT_SIMPLE_VISIT                           \
   };                                                          \
} }

#define EOS_TYPES_REFLECT_VECTOR(C)                            \
namespace eos { namespace types {                              \
   template<typename T>                                        \
   struct reflector<C<T>>                                      \
   {                                                           \
      using is_defined = std::true_type;                       \
      using is_struct  = std::false_type;                      \
      using is_vector  = std::true_type;                       \
      using type = C<T>;                                       \
      EOS_TYPES_REFLECT_SIMPLE_VISIT                           \
   };                                                          \
} }

#define EOS_TYPES_REFLECT_OPTIONAL(C)                          \
namespace eos { namespace types {                              \
   template<typename T>                                        \
   struct reflector<C<T>>                                      \
   {                                                           \
      using is_defined  = std::true_type;                      \
      using is_struct   = std::false_type;                     \
      using is_optional = std::true_type;                      \
      using type = C<T>;                                       \
      EOS_TYPES_REFLECT_SIMPLE_VISIT                           \
   };                                                          \
} }

#define EOS_TYPES_REFLECT_INDEX_TYPE_u_asc true, true
#define EOS_TYPES_REFLECT_INDEX_TYPE_u_desc true, false
#define EOS_TYPES_REFLECT_INDEX_TYPE_nu_asc false, true
#define EOS_TYPES_REFLECT_INDEX_TYPE_nu_desc false, false

#define EOS_TYPES_REFLECT_VISIT_INDEX(r, data, i, index)                                                     \
   { vector<uint16_t> mapping BOOST_PP_TUPLE_ELEM(3, 2, index);                                              \
     _v.TEMPLATE operator()<BOOST_PP_TUPLE_ELEM(3, 0, index)>(                                               \
                    BOOST_PP_CAT(EOS_TYPES_REFLECT_INDEX_TYPE_,BOOST_PP_TUPLE_ELEM(3, 1, index)), mapping ); \
   }

#define EOS_TYPES_REFLECT_VISIT_TABLE_END(T) \
  _v.TEMPLATE operator()<T>(true);

#define EOS_TYPES_CREATE_TABLE(T, INDICES)                                                                   \
namespace eos { namespace types {                                                                            \
   template <>                                                                                               \
   struct table_reflector<typename                                                                           \
      std::enable_if<eos::types::reflector<T>::is_struct::value, T>::type>                                   \
   {                                                                                                         \
      using is_defined = std::true_type;                                                                     \
      using type       = T;                                                                                  \
      template<typename Visitor>                                                                             \
      static void visit(Visitor& _v)                                                                         \
      {                                                                                                      \
         BOOST_PP_SEQ_FOR_EACH_I(EOS_TYPES_REFLECT_VISIT_INDEX, 0, INDICES)                                  \
         EOS_TYPES_REFLECT_VISIT_TABLE_END(T)                                                                \
      }                                                                                                      \
      static inline uint32_t register_table(eos::types::abi_constructor& ac,                                 \
                                            const vector<ABI::table_index>& indices)                         \
      {                                                                                                      \
         return ac.add_table(BOOST_PP_STRINGIZE(T), indices);                                                \
      }                                                                                                      \
  };                                                                                                         \
} }                                                                           

#define EOS_TYPES_REFLECT_REGISTER_TABLE(r, v, t)  eos::types::table_reflector<t>::visit(v);

#define EOS_TYPES_REFLECT_REGISTER_STRUCT(r, v, s) eos::types::reflector<s>::visit(v);

#define EOS_TYPES_REGISTER_TYPES_1 

#define EOS_TYPES_REGISTER_TYPES_2(name, tables) EOS_TYPES_REGISTER_TYPES_3(name, tables, BOOST_PP_SEQ_NIL)

#define EOS_TYPES_REGISTER_TYPES_3(name, tables, structs)                                                    \
namespace eos { namespace types {                                                                            \
   template <>                                                                                               \
   struct types_initializer<name>                                                                            \
   {                                                                                                         \
      static abi_constructor init()                                                                          \
      {                                                                                                      \
         abi_constructor ac;                                                                                 \
         type_discovery_visitor vis(ac);                                                                     \
         BOOST_PP_SEQ_FOR_EACH(EOS_TYPES_REFLECT_REGISTER_TABLE,  vis, tables)                               \
         BOOST_PP_SEQ_FOR_EACH(EOS_TYPES_REFLECT_REGISTER_STRUCT, vis, structs)                              \
         return ac;                                                                                          \
      }                                                                                                      \
   };                                                                                                        \
} } 

#if !BOOST_PP_VARIADICS_MSVC
#define EOS_TYPES_REGISTER_TYPES(...) \
   BOOST_PP_OVERLOAD(EOS_TYPES_REGISTER_TYPES_,__VA_ARGS__)(__VA_ARGS__)
#else // for Visual C++
#define EOS_TYPES_REGISTER_TYPES(...) \
   BOOST_PP_CAT(BOOST_PP_OVERLOAD(EOS_TYPES_REGISTER_TYPES_,__VA_ARGS__)(__VA_ARGS__),BOOST_PP_EMPTY())
#endif


namespace eos { namespace types {
   template<typename T>
   struct reflector
   {
      using is_defined = std::false_type;
   };

   template<typename T>
   struct table_reflector
   {
      using is_defined = std::false_type;
   };

   template<typename T>
   struct types_initializer
   {
   };

   struct rational
   {
      rational()
         : numerator(0), denominator(1)
      {}

      rational(int64_t n, uint64_t d)
         : numerator(n), denominator(d)
      {}

      int64_t  numerator;
      uint64_t denominator;
   };

   uint32_t register_struct(abi_constructor& ac, const std::vector<type_id>& field_types, const char* struct_name, 
                            const char* const field_names[], uint32_t field_names_length,
                            const char* const sorted_member_names[], bool const sorted_member_dir[], uint32_t sorted_member_count,
                            const char* base_name = nullptr);

} }


EOS_TYPES_REFLECT_BUILTIN(bool,     builtin_bool)
EOS_TYPES_REFLECT_BUILTIN(char,     builtin_uint8)
EOS_TYPES_REFLECT_BUILTIN(int8_t,   builtin_int8)
EOS_TYPES_REFLECT_BUILTIN(uint8_t,  builtin_uint8)
EOS_TYPES_REFLECT_BUILTIN(int16_t,  builtin_int16)
EOS_TYPES_REFLECT_BUILTIN(uint16_t, builtin_uint16)
EOS_TYPES_REFLECT_BUILTIN(int32_t,  builtin_int32)
EOS_TYPES_REFLECT_BUILTIN(uint32_t, builtin_uint32)
EOS_TYPES_REFLECT_BUILTIN(int64_t,  builtin_int64)
EOS_TYPES_REFLECT_BUILTIN(uint64_t, builtin_uint64)
EOS_TYPES_REFLECT_BUILTIN(std::string, builtin_string)
EOS_TYPES_REFLECT_BUILTIN(std::vector<uint8_t>, builtin_bytes)
EOS_TYPES_REFLECT_BUILTIN(eos::types::rational, builtin_rational)
EOS_TYPES_REFLECT_BUILTIN(eos::types::type_id, builtin_uint32)

EOS_TYPES_REFLECT_ARRAY(std::array)
EOS_TYPES_REFLECT_VECTOR(std::vector)
EOS_TYPES_REFLECT_OPTIONAL(boost::optional)

EOS_TYPES_REFLECT_TUPLE(std::pair, 2)
EOS_TYPES_REFLECT_TUPLE(std::tuple, 2)
EOS_TYPES_REFLECT_TUPLE(std::tuple, 3)
EOS_TYPES_REFLECT_TUPLE(std::tuple, 4)
EOS_TYPES_REFLECT_TUPLE(std::tuple, 5)
EOS_TYPES_REFLECT_TUPLE(std::tuple, 6)
EOS_TYPES_REFLECT_TUPLE(std::tuple, 7)

namespace eos { namespace types {

   using std::vector;
   using std::pair;

   struct type_discovery_visitor
   {
      abi_constructor& ac;
      type_id          tid;
      vector<type_id>  field_types;
      vector<ABI::table_index> indices;

      type_discovery_visitor( abi_constructor& ac)
         : ac(ac)
      {}

      template<typename B>
      typename std::enable_if<eos::types::reflector<B>::is_builtin::value>::type
      operator()()
      {
         tid = type_id(eos::types::reflector<B>::builtin_type);
      }

      template<typename B>
      typename std::enable_if<eos::types::reflector<B>::is_builtin::value>::type
      operator()(bool unique, bool ascending, const vector<uint16_t>& mapping) // Table index of builtin key type
      {
         tid = type_id(eos::types::reflector<B>::builtin_type);
         indices.push_back(ABI::table_index{ .key_type = tid, .unique = unique, .ascending = ascending, .mapping =  mapping });
      }

      template<class Class>
      typename std::enable_if<eos::types::reflector<Class>::is_struct::value>::type
      operator()(bool unique, bool ascending, const vector<uint16_t>& mapping) // Table index of struct key type
      {
         auto cur_struct_index = ac.get_struct_index(eos::types::reflector<Class>::name());
         if( cur_struct_index >= 0 )
         {
            tid = type_id::make_struct(static_cast<uint32_t>(cur_struct_index));
         }
         else
         {
            type_discovery_visitor vis(ac);
            eos::types::reflector<Class>::visit(vis);
            tid = vis.tid;
         }
         indices.push_back(ABI::table_index{ .key_type = tid, .unique = unique, .ascending = ascending, .mapping =  mapping });
      }

      template<class Class>
      typename std::enable_if<eos::types::reflector<Class>::is_struct::value>::type
      operator()(bool) // Called after processing the indices of the table.
      {
         auto cur_struct_index = ac.get_struct_index(eos::types::reflector<Class>::name());
         if( cur_struct_index < 0 )
         {
            type_discovery_visitor vis(ac);
            eos::types::reflector<Class>::visit(vis);
         }
         eos::types::table_reflector<Class>::register_table(ac, indices);
         indices.clear();
      }

      template<class Class>
      typename std::enable_if<eos::types::reflector<Class>::is_struct::value, bool>::type
      operator()(const char* name) // Called before processing the fields of a product type such as a struct or its base or a tuple/pair.
      {
         auto cur_struct_index = ac.get_struct_index(eos::types::reflector<Class>::name());
         if( cur_struct_index >= 0 )
         {
            tid = type_id::make_struct(static_cast<uint32_t>(cur_struct_index));
            return false; // Do not bother processing this struct because it has already been processed or is in the middle of being processed..
         }
         ac.declare_struct(eos::types::reflector<Class>::name());
         return true;
      }

      template<class Class, class Base>
      typename std::enable_if<eos::types::reflector<Class>::is_struct::value && eos::types::reflector<Base>::is_struct::value>::type
      operator()()const
      {
         if( ac.get_struct_index(eos::types::reflector<Base>::name()) >= 0 )
            return;

         type_discovery_visitor vis(ac);
         eos::types::reflector<Base>::visit(vis);
      }

      template<class Class>
      typename std::enable_if<eos::types::reflector<Class>::is_struct::value>::type
      operator()() // Called after processing the fields of the product type
      {
         tid = type_id::make_struct(eos::types::reflector<Class>::register_struct(ac, field_types));
         field_types.clear();
      }

      template<typename Member, class Class, Member (Class::*member)>
      typename std::enable_if<eos::types::reflector<Class>::is_struct::value && eos::types::reflector<Member>::is_struct::value>::type
      operator()(const char* name, uint32_t member_index)
      {
         auto index = ac.get_struct_index(eos::types::reflector<Member>::name());
        
         if( index >= 0 )
         {
            field_types.push_back(type_id::make_struct(index));
            return;
         }

         type_discovery_visitor vis(ac); 
         eos::types::reflector<Member>::visit(vis);
         field_types.push_back(vis.tid);
      }

      template<typename Member, class Class, Member (Class::*member)>
      typename std::enable_if<eos::types::reflector<Class>::is_struct::value && !eos::types::reflector<Member>::is_struct::value>::type
      operator()(const char* name, uint32_t member_index)
      {
         type_discovery_visitor vis(ac);
         eos::types::reflector<Member>::visit(vis);
         field_types.push_back(vis.tid);
      }

      template<typename Member, class Class, size_t Index> // Meant for tuples/pairs
      typename std::enable_if<eos::types::reflector<Class>::is_struct::value && eos::types::reflector<Member>::is_struct::value>::type
      operator()()
      {
         auto index = ac.get_struct_index(eos::types::reflector<Member>::name());
        
         if( index >= 0 )
         {
            field_types.push_back(type_id::make_struct(index));
            return;
         }

         type_discovery_visitor vis(ac); 
         eos::types::reflector<Member>::visit(vis);
         field_types.push_back(vis.tid);
      }

      template<typename Member, class Class, size_t Index> // Meant for tuples/pairs
      typename std::enable_if<eos::types::reflector<Class>::is_struct::value && !eos::types::reflector<Member>::is_struct::value>::type
      operator()()
      {
         type_discovery_visitor vis(ac);
         eos::types::reflector<Member>::visit(vis);
         field_types.push_back(vis.tid);
      }


      template<class Container>
      typename std::enable_if<eos::types::reflector<Container>::is_array::value>::type
      operator()()
      {
         static_assert( static_cast<uint32_t>(eos::types::reflector<Container>::num_elements) >= 2, 
                        "Arrays must have at least 2 elements." );
         static_assert( static_cast<uint32_t>(eos::types::reflector<Container>::num_elements) < type_id::array_size_limit,
                        "Array has too many elements." );

         auto num_elements = static_cast<uint32_t>(eos::types::reflector<Container>::num_elements);      

         type_discovery_visitor vis(ac);
         eos::types::reflector<typename Container::value_type>::visit(vis);
         
         switch( vis.tid.get_type_class() )
         {
            case type_id::builtin_type:
               if( num_elements < type_id::small_builtin_array_limit )
               {
                  tid = type_id(vis.tid.get_builtin_type(), num_elements);
                  return;
               }
               break;
            case type_id::struct_type:
               if( num_elements < type_id::small_array_limit )
               {
                  tid = type_id::make_struct(vis.tid.get_type_index(), num_elements);
                  return;
               }
               break;
            case type_id::array_type:
               if( num_elements < type_id::small_array_limit )
               {
                  tid = type_id::make_array(vis.tid.get_type_index(), num_elements);
                  return;
               }
               break;
            case type_id::vector_type:
               if( num_elements < type_id::small_array_limit )
               {
                  tid = type_id::make_vector(vis.tid.get_type_index(), num_elements);
                  return;
               }
               break;
            case type_id::variant_or_optional_type:
               if( num_elements < type_id::small_array_limit )
               {
                  tid = type_id::make_variant(vis.tid.get_type_index(), num_elements); // Also handles optionals
                  return;
               }
               break;
            case type_id::small_array_type:
            case type_id::small_array_of_builtins_type:
            case type_id::vector_of_something_type:
            case type_id::optional_struct_type:
               break;
         }
         
         tid = type_id::make_array(ac.add_array(vis.tid, num_elements));
      }

      template<class Container>
      typename std::enable_if<eos::types::reflector<Container>::is_vector::value>::type
      operator()()
      {
         type_discovery_visitor vis(ac);
         eos::types::reflector<typename Container::value_type>::visit(vis);
         
         switch( vis.tid.get_type_class() )
         {
            case type_id::builtin_type:
            {
               auto bt = vis.tid.get_builtin_type();
               if( bt == type_id::builtin_uint8 )
                  tid = type_id(type_id::builtin_bytes);
               else
                  tid = type_id::make_vector_of_builtins(bt);
               return;
            }
            case type_id::small_array_of_builtins_type:
               tid = type_id::make_vector_of_builtins(vis.tid.get_element_type().get_builtin_type(), static_cast<uint8_t>(vis.tid.get_small_array_size()));
               return;
            case type_id::struct_type:
               tid = type_id::make_vector_of_structs(vis.tid.get_type_index());
               return;
            case type_id::array_type:
            case type_id::small_array_type:
            case type_id::vector_type:
            case type_id::variant_or_optional_type:
            case type_id::vector_of_something_type:
            case type_id::optional_struct_type:
               break;
         }
         
         tid = type_id::make_vector(ac.add_vector(vis.tid));
      }

      template<class Container>
      typename std::enable_if<eos::types::reflector<Container>::is_optional::value>::type
      operator()()
      {
         type_discovery_visitor vis(ac);
         eos::types::reflector<typename Container::value_type>::visit(vis);
         
         switch( vis.tid.get_type_class() )
         {
            case type_id::struct_type:
               tid = type_id::make_optional_struct(vis.tid.get_type_index());
               return;
            case type_id::builtin_type:
            case type_id::small_array_of_builtins_type:
            case type_id::small_array_type:
            case type_id::array_type:
            case type_id::vector_type:
            case type_id::variant_or_optional_type:
            case type_id::vector_of_something_type:
            case type_id::optional_struct_type:
               break;
         }
         
         tid = type_id::make_optional(ac.add_optional(vis.tid));
      }

   };

} }

