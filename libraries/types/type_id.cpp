#include <eos/types/type_id.hpp>

#include <ostream>
#include <iomanip>
#include <stdexcept>
#include <boost/io/ios_state.hpp>

namespace eos { namespace types {
   
   type_id::type_id(uint32_t storage, bool skip_validation)
      : _storage(storage)
   {
      if( skip_validation )
         return;

      _storage = type_id(storage).get_storage();
   }

   type_id::type_id(uint32_t storage) 
      : _storage(storage) 
   {
      if( storage < variant_case_limit )
      {
         _storage = 0;
         return;
      }
      
      if( small_array_size_window::get(_storage) != 0 )
      {
         if( index_window::get(_storage) >= type_index_limit )
            _storage = 0;
         return;
      }

      switch( static_cast<special_extra>(extra_metadata_window::get(_storage)) )
      {
         case array_of_builtins:
         case vector_of_array_of_builtins:
            break;
         case vector_of_structs:
         case optional_struct:
            return; 
      }

      if( builtin_type_window::get(_storage) >= builtin_types_size )
        _storage = 0; 
   }

   type_id::type_id() 
      : _storage(0)
   {} 

   type_id::type_id(builtin b, uint8_t num_elements)
      : _storage(0)
   { 
      if( num_elements == 0 )
         throw std::domain_error("Number of elements cannot be 0.");

      //if( num_elements >= small_builtin_array_limit ) // Redundant since the limit is the natural limit of the uint8_t
      //   throw std::domain_error("The maximum number of elements for a small array of builtins is 255. For larger arrays, use the make_array function."); 

      small_builtin_array_size_window::set(_storage, num_elements);
      builtin_type_window::set(_storage, static_cast<uint16_t>(b));
   } 

   inline void check_index_limit(type_id::index_t index)
   {
      if( index >= type_id::type_index_limit )
         throw std::domain_error("Index is too large.");
   }

   inline void check_arguments(type_id::index_t index, uint8_t num_elements)
   {
      check_index_limit(index);

      if( num_elements == 0 )
         throw std::domain_error("Number of elements cannot be 0.");

      if( num_elements >= type_id::small_array_limit )
         throw std::domain_error("The maximum number of elements for a small array is 31. For larger arrays, use the make_array function.");
   }

   type_id type_id::make_struct(index_t index, uint8_t num_elements)
   {
      check_arguments(index, num_elements);
      uint32_t storage = 0;

      small_array_size_window::set(storage, num_elements);
      extra_metadata_window::set(storage, static_cast<uint8_t>(struct_index)); 
      index_window::set(storage, index);
      return {storage, true};
   }

   type_id type_id::make_array(index_t index, uint8_t num_elements)
   {
      check_arguments(index, num_elements);
      uint32_t storage = 0;

      small_array_size_window::set(storage, num_elements);
      extra_metadata_window::set(storage, static_cast<uint8_t>(array_index)); 
      index_window::set(storage, index);
      return {storage, true};
   }

   type_id type_id::make_vector(index_t index, uint8_t num_elements)
   {
      check_arguments(index, num_elements);
      uint32_t storage = 0;
      
      small_array_size_window::set(storage, num_elements);
      extra_metadata_window::set(storage, static_cast<uint8_t>(vector_index)); 
      index_window::set(storage, index);
      return {storage, true};
   }

   type_id type_id::make_variant(index_t index, uint8_t num_elements)
   {
      check_arguments(index, num_elements);
      uint32_t storage = 0;

      small_array_size_window::set(storage, num_elements);
      extra_metadata_window::set(storage, static_cast<uint8_t>(sum_type_index)); 
      index_window::set(storage, index);
      return {storage, true};
   }

   type_id type_id::make_optional(index_t index, uint8_t num_elements)
   {
      return make_variant(index, num_elements);
   }

   type_id type_id::make_optional_struct(index_t index)
   {
      check_index_limit(index);
      uint32_t storage = 0;

      extra_metadata_window::set(storage, static_cast<uint8_t>(optional_struct));
      index_window::set(storage, index);
      return {storage, true};
   }

   type_id type_id::make_vector_of_structs(index_t index)
   {
      check_index_limit(index);
      uint32_t storage = 0;

      extra_metadata_window::set(storage, static_cast<uint8_t>(vector_of_structs));
      index_window::set(storage, index);
      return {storage, true};
   }

   type_id type_id::make_vector_of_builtins(builtin b, uint8_t num_elements)
   {
      uint32_t storage = type_id(b, num_elements).get_storage();   

      extra_metadata_window::set(storage, static_cast<uint8_t>(vector_of_array_of_builtins));
      return {storage, true};
   }

   type_id::size_align type_id::get_builtin_type_size_align(builtin bt) 
   {
      if( static_cast<uint16_t>(bt) >= static_cast<uint16_t>(builtin_types_size) )
         throw std::invalid_argument("Not a valid builtin type");

      uint8_t size_and_align = 8; // Default word size
      switch(bt)
      {
         case builtin_int8:
         case builtin_uint8:
            size_and_align = 1;
            break;
         case builtin_int16:
         case builtin_uint16:
            size_and_align = 2;
            break;
         case builtin_int32:
         case builtin_uint32:
            size_and_align = 4;
            break;
         case builtin_int64:
         case builtin_uint64:
            size_and_align = 8;
            break;
         case builtin_bool:
            return {1, 0}; // In the case of bool, size is in bits
         case builtin_string:
         case builtin_bytes:
         case builtin_any:
            break;
         case builtin_rational:
            return {16, 8};
      }

      return {size_and_align, size_and_align};
   }

   const char* type_id::get_builtin_type_name(builtin bt)
   {
      if( static_cast<uint16_t>(bt) >= static_cast<uint16_t>(builtin_types_size) )
         throw std::invalid_argument("Not a valid builtin type");

      switch(bt)
      {
         case builtin_int8:
            return "Int8";
         case builtin_uint8:
            return "UInt8";
         case builtin_int16:
            return "Int16";
         case builtin_uint16:
            return "UInt16";
         case builtin_int32:
            return "Int32";
         case builtin_uint32:
            return "UInt32";
         case builtin_int64:
            return "Int64";
         case builtin_uint64:
            return "UInt64";
         case builtin_bool:
            return "Bool";
         case builtin_string:
            return "String";
         case builtin_bytes:
            return "Bytes";
         case builtin_rational:
            return "Rational";
         case builtin_any:
            return "Any";
      }

      return "ERROR"; // Should never actually be reached, but added to shut up compiler warning
   }

   type_id::type_class type_id::get_type_class()const
   {
      if( is_void() )
         throw std::invalid_argument("Cannot call get_type_class on a Void type");

      uint32_t size = small_array_size_window::get(_storage);
      if( size > 1 )
         return small_array_type;
      else if (size == 1)
      {
         switch( static_cast<index_class>(extra_metadata_window::get(_storage)) )
         {
            case struct_index:
               return struct_type;
            case array_index:
               return array_type;
            case vector_index:
               return vector_type;
            case sum_type_index:
               return variant_or_optional_type;
         }
      }

      switch( static_cast<special_extra>(extra_metadata_window::get(_storage)) )
      {
         case array_of_builtins:
            break;
         case vector_of_array_of_builtins:
         case vector_of_structs:
            return vector_of_something_type;
         case optional_struct:
            return optional_struct_type;
      }

      size = small_builtin_array_size_window::get(_storage);
      if( size > 1 )
         return small_array_of_builtins_type;
      else if( size == 1 )
         return builtin_type;
      
      throw std::runtime_error("Invariant failure: somehow a type_id with reserved representation was created.");
   }

   type_id::builtin type_id::get_builtin_type()const
   {
      if( is_void() || small_array_size_window::get(_storage) > 0 || extra_metadata_window::get(_storage) >= 2 )
         throw std::logic_error("This type does not contain a builtin.");

      return static_cast<builtin>(builtin_type_window::get(_storage));
   }

   type_id::index_t type_id::get_type_index()const
   {
      if( small_array_size_window::get(_storage) == 0 && extra_metadata_window::get(_storage) < 2 )
         throw std::logic_error("This type does not contain an index.");

      return index_window::get(_storage);
   }

   void type_id::set_type_index(index_t index)
   {
     if( small_array_size_window::get(_storage) == 0 && extra_metadata_window::get(_storage) < 2 )
         throw std::logic_error("This type does not contain an index.");

     check_index_limit(index);
     
     index_window::set(_storage, index);
   }


   uint32_t type_id::get_small_array_size()const // Returns 1 if the type is not a small array. Never returns 0.
   {
      uint32_t size = small_array_size_window::get(_storage);
      if( size > 0 )
         return size;

      size = small_builtin_array_size_window::get(_storage);
      if( size > 0 )
         return size;

      if( extra_metadata_window::get(_storage) > 0 )
         return 1;
      
      if( is_void() )
         throw std::logic_error("Type is void.");

      throw std::runtime_error("Invariant failure: somehow a type_id with reserved representation was created.");
   }

   type_id type_id::get_element_type()const // Return Void if it is not possible to find the element type
   {
      uint32_t size = small_array_size_window::get(_storage);
      if( size > 1 )
      {
         uint32_t storage = _storage;
         small_array_size_window::set(storage, 1);
         return {storage};
      }
      else if (size == 1)
      {
         return {};
      }

      switch( static_cast<special_extra>(extra_metadata_window::get(_storage)) )
      {
         case array_of_builtins:
            if( small_builtin_array_size_window::get(_storage) <= 1 )
               return {};
            return type_id(static_cast<builtin>(builtin_type_window::get(_storage)));
         case vector_of_array_of_builtins:
            break;
         case vector_of_structs:
         case optional_struct:
            return type_id::make_struct(index_window::get(_storage));
      }

      uint32_t storage = _storage;
      extra_metadata_window::set(storage, 0);
      return {storage};
   }

   void print_builtin(std::ostream& os, type_id tid)
   {
      auto tc = tid.get_type_class();
      
      if( tc == type_id::builtin_type )
      {
         os << type_id::get_builtin_type_name(tid.get_builtin_type());
         return;
      }

      if( tc == type_id::small_array_of_builtins_type )
      {
         auto element_tid = tid.get_element_type();
         if( element_tid.get_builtin_type() == type_id::builtin_bool )
         {
            os << "Bitset<" << tid.get_small_array_size() << ">";
            return;
         }
         os << "Array<" << type_id::get_builtin_type_name(element_tid.get_builtin_type()) << ", ";
         os << tid.get_small_array_size() << ">";
         return;
      }
   
      throw std::invalid_argument("This function only prints builtins and small arrays of builtins");
   }

   std::ostream& operator<<(std::ostream& os, const type_id& tid)
   {
      boost::io::ios_flags_saver ifs( os );

      os << std::dec;
      
      if( tid.is_void() )
      {
         os << "Void";
         return os;
      }

      auto tc = tid.get_type_class();
      switch( tc )
      {
         case type_id::builtin_type:
         case type_id::small_array_of_builtins_type:
            print_builtin(os, tid);
            return os;
         case type_id::small_array_type:
            os << "Array<T, ";
            os << tid.get_small_array_size();
            os << ">";
            tc = tid.get_element_type().get_type_class();
            break;
         case type_id::vector_of_something_type:
         {
            auto element_tid = tid.get_element_type();
            switch( element_tid.get_type_class() )
            {
               case type_id::builtin_type:
               case type_id::small_array_of_builtins_type:
                  os << "Vector<";
                  print_builtin(os, element_tid);
                  os << ">";
                  return os;
               case type_id::struct_type:
                  os << "Vector<T>";
                  tc = type_id::struct_type;
                  break;
               default:
                  throw std::runtime_error("Unexpected result: should not have encountered this type class while writing type to output.");
            }
            break;
         }
         case type_id::optional_struct_type:
            os << "Optional<T>";
            break;
         case type_id::vector_type:
         case type_id::array_type:
         case type_id::struct_type:
         case type_id::variant_or_optional_type:
            os << "T";
            break;
      }

      os << " [T = ";
      switch( tc )
      {
         case type_id::optional_struct_type:
         case type_id::struct_type:
            os << "struct";
            break;
         case type_id::array_type:
            os << "array";
            break;
         case type_id::vector_type:
            os << "vector";
            break;
         case type_id::variant_or_optional_type:
            os << "sum_type";
            break;
         default:
            throw std::runtime_error("Unexpected result: should not have encountered this type class while writing type to output.");
      }
      os << "(" << tid.get_type_index() << ")]";
      return os;
   }

   // Sorts in order of descending alignment and descending size (in that order of priority).
   // However, alignments of 0 or 1 are treated the same.
   bool operator<(type_id::size_align lhs, type_id::size_align rhs)
   {
      using size_align = type_id::size_align;

      auto lhs_align = size_align::align_window::get(lhs._storage);
      auto rhs_align = size_align::align_window::get(rhs._storage);
      if( (lhs_align == 0 || lhs_align == 1) && rhs_align > 1 )
         return false;
      else if( lhs_align > 1 && (rhs_align == 0 || rhs_align == 1) )
         return true;
      else if( !(lhs_align > 1 && rhs_align > 1) )
      {
         lhs_align = 1;
         rhs_align = 1;
      }
      // TODO: Handle bool arrays properly.      
      return std::make_tuple(lhs_align, size_align::size_window::get(lhs._storage)) > std::make_tuple(rhs_align, size_align::size_window::get(rhs._storage));
   }


} }

