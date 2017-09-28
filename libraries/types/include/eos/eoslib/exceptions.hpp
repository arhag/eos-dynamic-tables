#pragma once

#ifdef EOS_TYPES_FULL_CAPABILITY

#include <stdexcept>

#define EOS_ERROR(type, message) do { throw type(message); } while(0)

#else

//void assert( unsigned int test, const char* cstr );

//#define EOS_ERROR(type, message) do { assert(0, message); } while(0)

#include <cassert>

#define EOS_ERROR(type, message) do { assert(0); } while(0)

#endif
