cmake_minimum_required( VERSION 2.8.12 )

set( CMAKE_CXX_STANDARD 14 )

# http://stackoverflow.com/a/18369825
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(FATAL_ERROR "GCC version must be at least 4.8!")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
        message(FATAL_ERROR "Clang version must be at least 3.3!")
    endif()
endif()

set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wall" )

if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
   if( CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.0.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.0.0 )
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-partial-specialization" )
   endif()
endif()

if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
   set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp" )
endif()

add_subdirectory( libraries )
add_subdirectory( programs )

