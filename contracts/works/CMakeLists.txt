add_contract(works works ${CMAKE_CURRENT_SOURCE_DIR}/src/works.cpp)

target_include_directories(works
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/telos-decide/contracts/trail/include)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

set_target_properties(works
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/resources/works.contracts.md ${CMAKE_CURRENT_BINARY_DIR}/resources/works.contracts.md @ONLY )

target_compile_options( works PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/resources -R${CMAKE_CURRENT_BINARY_DIR}/resources )
