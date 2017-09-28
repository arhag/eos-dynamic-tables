#pragma once

#include <eos/eoslib/type_id.hpp>
#include <eos/eoslib/type_traits.hpp>

#include <string>
#include <array>
#include <vector>
#include <utility>
#include <tuple>   

using eoslib::true_type;
using eoslib::false_type;

namespace eos { namespace types {
   template<typename T>
   struct reflector
   {
      using is_defined = false_type;
   };

   template<typename T>
   struct table_reflector
   {
      using is_defined = false_type;
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

} }

#ifndef _MSC_VER
  #define TEMPLATE template
#else
  // Disable warning C4482: nonstandard extention used: enum 'enum_type::enum_value' used in qualified name
  #pragma warning( disable: 4482 )
  #define TEMPLATE
#endif

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
      using is_defined      = true_type;                       \
      using is_struct       = false_type;                      \
      using is_builtin      = true_type;                       \
      using type            = T;                               \
      static const type_id::builtin builtin_type = type_id::B; \
      EOS_TYPES_REFLECT_SIMPLE_VISIT                           \
   };                                                          \
} }

#define EOS_TYPES_REFLECT_ARRAY(C)                             \
namespace eos { namespace types {                              \
   template<typename T, size_t N>                              \
   struct reflector<C<T, N>>                                   \
   {                                                           \
      using is_defined       = true_type;                      \
      using is_struct        = false_type;                     \
      using is_array         = true_type;                      \
      using type             = C<T, N>;                        \
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
      using is_defined       = true_type;                      \
      using is_struct        = false_type;                     \
      using is_vector        = true_type;                      \
      using type             = C<T>;                           \
      EOS_TYPES_REFLECT_SIMPLE_VISIT                           \
   };                                                          \
} }

#define EOS_TYPES_REFLECT_OPTIONAL(C)                          \
namespace eos { namespace types {                              \
   template<typename T>                                        \
   struct reflector<C<T>>                                      \
   {                                                           \
      using is_defined      = true_type;                       \
      using is_struct       = false_type;                      \
      using is_optional     = true_type;                       \
      using type            = C<T>;                            \
      EOS_TYPES_REFLECT_SIMPLE_VISIT                           \
   };                                                          \
} }

// Alternative to requiring Boost Preprocesser headers just to implement EOS_TYPES_REFLECT_TUPLE
// Adapted from: https://github.com/pfultz2/Cloak/wiki/C-Preprocessor-tricks,-tips,-and-idioms

#define CAT(a, ...) PRIMITIVE_CAT(a, __VA_ARGS__)
#define PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__

#define IIF(c) PRIMITIVE_CAT(IIF_, c)
#define IIF_0(t, ...) __VA_ARGS__
#define IIF_1(t, ...) t

#define COMPL(b) PRIMITIVE_CAT(COMPL_, b)
#define COMPL_0 1
#define COMPL_1 0

#define BITAND(x) PRIMITIVE_CAT(BITAND_, x)
#define BITAND_0(y) 0
#define BITAND_1(y) y

#define INC(x) PRIMITIVE_CAT(INC_, x)
#define INC_0 1
#define INC_1 2
#define INC_2 3
#define INC_3 4
#define INC_4 5
#define INC_5 6
#define INC_6 7
#define INC_7 8
#define INC_8 9
#define INC_9 9

#define DEC(x) PRIMITIVE_CAT(DEC_, x)
#define DEC_0 0
#define DEC_1 0
#define DEC_2 1
#define DEC_3 2
#define DEC_4 3
#define DEC_5 4
#define DEC_6 5
#define DEC_7 6
#define DEC_8 7
#define DEC_9 8

#define CHECK_N(x, n, ...) n
#define CHECK(...) CHECK_N(__VA_ARGS__, 0,)
#define PROBE(x) x, 1,

#define IS_PAREN(x) CHECK(IS_PAREN_PROBE x)
#define IS_PAREN_PROBE(...) PROBE(~)

#define NOT(x) CHECK(PRIMITIVE_CAT(NOT_, x))
#define NOT_0 PROBE(~)

#define BOOL(x) COMPL(NOT(x))
#define IF(c) IIF(BOOL(c))

#define EAT(...)
#define EXPAND(...) __VA_ARGS__
#define WHEN(c) IF(c)(EXPAND, EAT)

#define EMPTY()
#define DEFER(id) id EMPTY()
#define OBSTRUCT(...) __VA_ARGS__ DEFER(EMPTY)()
#define EXPAND(...) __VA_ARGS__

#define EVAL(...)  EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL1(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL2(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL3(...) EVAL4(EVAL4(EVAL4(__VA_ARGS__)))
#define EVAL4(...) EVAL5(EVAL5(EVAL5(__VA_ARGS__)))
#define EVAL5(...) __VA_ARGS__

#define REPEAT(count, macro, ...) \
    WHEN(count) \
    ( \
        OBSTRUCT(REPEAT_INDIRECT) () \
        ( \
            DEC(count), macro, __VA_ARGS__ \
        ) \
        OBSTRUCT(macro) \
        ( \
            DEC(count), __VA_ARGS__ \
        ) \
    )
#define REPEAT_INDIRECT() REPEAT

#define PRIMITIVE_COMPARE(x, y) IS_PAREN \
( \
COMPARE_ ## x ( COMPARE_ ## y) (())  \
)

#define IS_COMPARABLE(x) IS_PAREN( CAT(COMPARE_, x) (()) )

#define NOT_EQUAL(x, y) \
IIF(BITAND(IS_COMPARABLE(x))(IS_COMPARABLE(y)) ) \
( \
   PRIMITIVE_COMPARE, \
   1 EAT \
)(x, y)

#define EQUAL(x, y) COMPL(NOT_EQUAL(x, y))

#define COMPARE_0(x) x
#define COMPARE_1(x) x
#define COMPARE_2(x) x
#define COMPARE_3(x) x
#define COMPARE_4(x) x
#define COMPARE_5(x) x
#define COMPARE_6(x) x
#define COMPARE_7(x) x
#define COMPARE_8(x) x
#define COMPARE_9(x) x

#define REPEAT_FROM_TO(from, to, macro, ...) \
    WHEN(BITAND(NOT_EQUAL(from, to))(to)) \
    ( \
        OBSTRUCT(REPEAT_FROM_TO_INDIRECT) () \
        ( \
            from, DEC(to), macro, __VA_ARGS__ \
        ) \
        OBSTRUCT(macro) \
        ( \
            DEC(to), __VA_ARGS__ \
        ) \
    )
#define REPEAT_FROM_TO_INDIRECT() REPEAT_FROM_TO

#define EOS_TYPES_REFLECT_TEMPLATE_ARG( index, data ) , data T##index

#define EOS_TYPES_REFLECT_VISIT_TUPLE_ELEMENT( index, data )                                                 \
         _v.TEMPLATE operator()<data##index, type, index>( _s );

#define EOS_TYPES_REFLECT_VISIT_TUPLE_ELEMENT_TYPE( index, data )                                            \
         _v.TEMPLATE operator()<data##index, type, index>();

#define EOS_TYPES_REFLECT_TUPLE(C, num_template_args)                                                        \
namespace eos { namespace types {                                                                            \
   template<typename T0                                                                                      \
            EVAL(REPEAT_FROM_TO(1, num_template_args, EOS_TYPES_REFLECT_TEMPLATE_ARG, typename))>            \
   struct reflector<C<T0                                                                                     \
                      EVAL(REPEAT_FROM_TO(1, num_template_args, EOS_TYPES_REFLECT_TEMPLATE_ARG, ))>>         \
   {                                                                                                         \
      using is_defined      = std::true_type;                                                                \
      using is_product_type = std::true_type;                                                                \
      using is_struct       = std::false_type;                                                               \
      using is_tuple        = std::true_type;                                                                \
      using type            = C<T0 EVAL(REPEAT_FROM_TO(1, num_template_args,                                 \
                                                           EOS_TYPES_REFLECT_TEMPLATE_ARG, ))>;              \
      enum  field_count_enum {                                                                               \
         field_count = num_template_args                                                                     \
      };                                                                                                     \
      enum sorted_member_count_enum {                                                                        \
         sorted_member_count = num_template_args                                                             \
      };                                                                                                     \
      template<typename Visitor>                                                                             \
      static void visit(Visitor& _v)                                                                         \
      {                                                                                                      \
         EVAL(REPEAT(num_template_args, EOS_TYPES_REFLECT_VISIT_TUPLE_ELEMENT_TYPE, T))                      \
         _v.TEMPLATE operator()<type>();                                                                     \
      }                                                                                                      \
      template<typename Visitor>                                                                             \
      static void visit(const type& _s, const Visitor& _v)                                                   \
      {                                                                                                      \
         EVAL(REPEAT(num_template_args, EOS_TYPES_REFLECT_VISIT_TUPLE_ELEMENT, T))                           \
      }                                                                                                      \
      template<typename Visitor>                                                                             \
      static void visit(type& _s, const Visitor& _v)                                                         \
      {                                                                                                      \
         EVAL(REPEAT(num_template_args, EOS_TYPES_REFLECT_VISIT_TUPLE_ELEMENT, T))                           \
      }                                                                                                      \
   };                                                                                                        \
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
//EOS_TYPES_REFLECT_OPTIONAL(boost::optional)

EOS_TYPES_REFLECT_TUPLE(std::pair, 2)
EOS_TYPES_REFLECT_TUPLE(std::tuple, 2)
EOS_TYPES_REFLECT_TUPLE(std::tuple, 3)
EOS_TYPES_REFLECT_TUPLE(std::tuple, 4)
EOS_TYPES_REFLECT_TUPLE(std::tuple, 5)
EOS_TYPES_REFLECT_TUPLE(std::tuple, 6)
EOS_TYPES_REFLECT_TUPLE(std::tuple, 7)


#define EOS_TYPES_REFLECT_STRUCT(...)
#define EOS_TYPES_REFLECT_STRUCT_DERIVED(...)
#define EOS_TYPES_CREATE_TABLE(...)
#define EOS_TYPES_REGISTER_TYPES(...) 


