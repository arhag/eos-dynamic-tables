
file(GLOB HEADERS  "${CMAKE_CURRENT_SOURCE_DIR}/include/eos/types/*.hpp")

add_definitions(-DBOOST_PP_VARIADICS)

add_library( eos_types 
             reflect.cpp 
             raw_region.cpp 
             type_id.cpp 
             field_metadata.cpp 
             types_manager.cpp 
             abi_constructor.cpp 
             types_constructor.cpp 
             ${HEADERS} 
           )
target_include_directories( eos_types PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
