add_executable( reflection_test1 reflection_test1.cpp )
target_link_libraries( reflection_test1 eos_types )

add_executable( reflection_test2 reflection_test2.cpp )
target_link_libraries( reflection_test2 eos_types )

add_executable( table_test1 table_test1.cpp )
target_link_libraries( table_test1 eos_table )
