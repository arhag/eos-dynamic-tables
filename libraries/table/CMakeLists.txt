
file(GLOB HEADERS  "${CMAKE_CURRENT_SOURCE_DIR}/include/eos/table/*.hpp")

add_definitions(-DBOOST_PP_VARIADICS)

add_library( eos_table
             dynamic_object.cpp 
             ${HEADERS} 
           )
target_include_directories( eos_table PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_link_libraries( eos_table eos_types )
