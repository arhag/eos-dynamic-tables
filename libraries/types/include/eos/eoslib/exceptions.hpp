#pragma once

#ifdef EOS_TYPES_FULL_CAPABILITY

#include <stdexcept>

#define EOS_ERROR(type, message) throw type(message)

#else

//void assert( unsigned int test, const char* cstr );

//#define EOS_ERROR(type, message) do { assert(0, message); } while(0)

#include <cassert>

#define EOS_ERROR(type, message) assert(0)

#endif

#define EOS_ASSERT(x, msg) ((x) ? (void)0 : EOS_ERROR(std::runtime_error, msg))
