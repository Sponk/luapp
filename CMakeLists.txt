cmake_minimum_required(VERSION 3.5)

project(Lua++)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

bison_target(parser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)
flex_target(lexer src/lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
add_flex_bison_dependency(lexer parser)

add_executable(l++ src/main.cpp ${BISON_parser_OUTPUTS} ${FLEX_lexer_OUTPUTS})

include_directories(${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader bitreader bitwriter passes target)
message("-- ${llvm_libs}")
target_link_libraries(l++ ${llvm_libs})
add_definitions(-std=c++11)
