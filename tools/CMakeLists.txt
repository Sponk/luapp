#llvm_map_components_to_libnames(llvm_libs)
#add_llvm_loadable_module(ExtractDefinitions ExtractDefinitions.cpp)

#find_path(LibClang_INCLUDE_DIR clang-c/Index.h)
#find_library(LIBCLANG_LIBRARY NAMES clang)
find_package(Clang REQUIRED clangTooling libClang)

add_executable(HeaderToModule HeaderToModule.cpp)

llvm_map_components_to_libnames(llvm_libs support)

add_definitions(${LLVM_DEFINITIONS})
add_definitions(${Clang_DEFINITIONS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIR})
target_link_libraries(HeaderToModule
	LLVM
	clangFrontend
	clangSerialization
       # clangDriver
       # clangParse
       # clangSema
       # clangAnalysis
	clangAST
       # clangBasic
       # clangEdit
       # clangLex
	clangTooling
)

