#pragma once

#include <eos/types/bit_view.hpp>

#include <iosfwd>

namespace eos { namespace types {

   class type_id
   {
   private:

      uint32_t _storage;

      using small_array_size_window         = bit_view<uint8_t,  26,  6, uint32_t>;
      using extra_metadata_window           = bit_view<uint8_t,  24,  2, uint32_t>; 
      using small_builtin_array_size_window = bit_view<uint8_t,  16,  8, uint32_t>;
      using builtin_type_window             = bit_view<uint16_t,  0, 16, uint32_t>;
      using index_window                    = bit_view<uint32_t,  0, 24, uint32_t>;

      enum special_extra : uint8_t
      {
         array_of_builtins = 0,
         vector_of_array_of_builtins,
         vector_of_structs,
         optional_struct
      };

      enum index_class : uint8_t
      {
         struct_index = 0,
         array_index,
         vector_index,
         sum_type_index
      };

      type_id(uint32_t storage, bool skip_validation); // Private constructor that avoids overhead of validation checks if skip_validation is true

   public:

      static inline uint32_t round_up_to_alignment(uint32_t n, uint8_t align)
      {
         auto remainder = n % align;
         auto rounded = n;  // Rounded up to nearest multiple of align
         if( remainder != 0)
            rounded += (align - remainder);
         return rounded;
      }

      class size_align
      {
      private:

         uint32_t _storage;
      
         using align_window = bit_view<uint8_t,  24,  8, uint32_t>;
         using size_window  = bit_view<uint32_t,  0, 24, uint32_t>; 

      public:
         
         size_align() : _storage(0) {}

         size_align(uint32_t storage) : _storage(storage) {}

         size_align(uint32_t size, uint8_t align)
            : _storage(0)
         {
            size_window::set(_storage, size);
            align_window::set(_storage, align);
         }   

         inline uint32_t get_storage()const { return _storage; }
         inline uint32_t get_size()const    { return size_window::get(_storage); }
         inline uint8_t  get_align()const   { return align_window::get(_storage); }
         inline uint32_t get_stride()const  { return round_up_to_alignment(get_size(), get_align()); }
         inline bool     is_complete()const { return size_window::get(_storage) > 0; }

         // Sorts in order of descending alignment and descending size (in that order of priority).
         // However, alignments of 0 or 1 are treated the same.
         friend bool operator<(size_align lhs, size_align rhs);

      };

      enum type_class : uint8_t 
      {
         builtin_type,
         struct_type,
         array_type,
         small_array_type,
         small_array_of_builtins_type,
         vector_type,
         vector_of_something_type,
         optional_struct_type,
         variant_or_optional_type
      };

      enum builtin : uint16_t // Maximum of 2^16 enumerants allowed
      {
         builtin_int8 = 0,
         builtin_uint8,
         builtin_int16,
         builtin_uint16,
         builtin_int32,
         builtin_uint32,
         builtin_int64,
         builtin_uint64,
         builtin_bool,
         builtin_string,
         builtin_bytes,
         builtin_rational,
         builtin_any       // Always keep builtin_any as the last enumerant so that the builting_types_size calculation below works correctly.
      };

      static const uint32_t builtin_types_size = static_cast<uint32_t>(builtin_any) + 1; // Should always be the number of enumerants in the builtin enum

      using index_t = uint32_t;

      static const index_t  type_index_limit    = ((1 << 24) - 1); // The reason for the minus one is so that type_index_limit can be used to represent no index in certain parts of the code.
      static const uint32_t variant_case_limit  = (1 << 16); // Stricter limit is needed to be able to distinguish optional from variant
      static const uint32_t struct_fields_limit = ((1 << 15) - 1); // Constraint imposed by int16_t sort order on fields / base. So that all valid structs can have sort orders specified on all their fields and their base.
      static const uint32_t array_size_limit    = (1 << 24);
      static const uint32_t small_builtin_array_limit = 256;
      static const uint8_t  small_array_limit = 64;
      
      type_id(); 
      explicit type_id(uint32_t storage); // Also makes it possible to deserialize type_id from a builtin UInt32
      type_id(builtin b, uint8_t num_elements = 1);
 
      static type_id make_struct(index_t index, uint8_t num_elements = 1);
      static type_id make_array(index_t index, uint8_t num_elements = 1);
      static type_id make_vector(index_t index, uint8_t num_elements = 1);
      static type_id make_variant(index_t index, uint8_t num_elements = 1);
      static type_id make_optional(index_t index, uint8_t num_elements = 1);
      static type_id make_optional_struct(index_t index);
      static type_id make_vector_of_structs(index_t index);
      static type_id make_vector_of_builtins(builtin b, uint8_t num_elements = 1);

      // The method is needed to serialize type_id as a builtin UInt32
      inline explicit operator uint32_t()const { return _storage; }

      static size_align  get_builtin_type_size_align(builtin bt); 
      static const char* get_builtin_type_name(builtin bt);

      type_class get_type_class()const;
      builtin    get_builtin_type()const;
      index_t    get_type_index()const;
      void       set_type_index(index_t index);
      uint32_t   get_small_array_size()const; // Returns 1 if the type is not a small array. Never returns 0.
      type_id    get_element_type()const; // Return Void if it is not possible to find the element type

      inline uint32_t get_storage()const
      {
         return _storage;
      }

      inline bool is_void()const
      {
         return (_storage == 0);
      }

      inline bool is_valid()const
      {
         return (_storage >= variant_case_limit);
      }

      friend class field_metadata; // So that field_metadata can quickly be constructed from raw storage without unnecessary validation.
      friend class types_manager;  // For the same reason.

      friend inline bool operator<(type_id lhs, type_id rhs) { return lhs.get_storage() < rhs.get_storage(); }
      friend inline bool operator==(type_id lhs, type_id rhs) { return lhs.get_storage() == rhs.get_storage(); }
      friend inline bool operator!=(type_id lhs, type_id rhs) { return lhs.get_storage() != rhs.get_storage(); }

      friend std::ostream& operator<<(std::ostream& os, const type_id&);

   };

} }

