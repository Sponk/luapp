
add_lpp_module(runtime runtime.lpp)
add_lpp_executable(runtime_test test/main.lpp)

add_dependencies(runtime l++)
add_dependencies(runtime_test runtime)
