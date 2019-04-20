cmake_minimum_required(VERSION 3.5)
project(Lua++)

set(BUILD_SHARED_LIBS ON)
set(LUAPP_COMPILER ${CMAKE_BINARY_DIR}/l++)
macro(add_lpp_executable target source)
    add_custom_target(${target} ALL COMMAND ${LUAPP_COMPILER} -s ${CMAKE_CURRENT_SOURCE_DIR}/${source} -o ${CMAKE_CURRENT_BINARY_DIR}/${target} -I ${CMAKE_CURRENT_BINARY_DIR})
endmacro()

macro(add_lpp_module target source)
    add_custom_target(${target} ALL COMMAND ${LUAPP_COMPILER} -m -s ${CMAKE_CURRENT_SOURCE_DIR}/${source} -o ${CMAKE_CURRENT_BINARY_DIR}/${target})
endmacro()

find_package(LLVM REQUIRED CONFIG)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(HandleLLVMOptions)
include(AddLLVM REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

bison_target(parser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)
flex_target(lexer src/lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
add_flex_bison_dependency(lexer parser)

add_executable(l++ src/main.cpp src/SemanticChecker.cpp ${BISON_parser_OUTPUTS} ${FLEX_lexer_OUTPUTS} src/MetaContext.cpp src/MetaContext.h)

target_include_directories(l++ PRIVATE ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader bitreader bitwriter passes target option)
message("-- ${llvm_libs}")
target_link_libraries(l++ PRIVATE ${llvm_libs})
add_definitions(-std=c++11)

add_subdirectory(tools)
add_subdirectory(runtime)
add_subdirectory(examples)

## Lua metaprogramming yay!
find_package(Lua REQUIRED)
find_package(SWIG REQUIRED)

include(${SWIG_USE_FILE})

set_property(SOURCE src/AST.i PROPERTY CPLUSPLUS ON)
swig_add_library(LuaAST
        TYPE STATIC
        LANGUAGE lua
        SOURCES src/AST.i)

target_include_directories(LuaAST PRIVATE src/)

target_link_libraries(l++ PRIVATE ${LUA_LIBRARIES} LuaAST)
target_include_directories(l++ PRIVATE ${LUA_INCLUDE_DIRS})
