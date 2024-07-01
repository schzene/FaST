
include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(accuracy_test)
add_subdirectory(protocols_test)

macro(add_test _name)
    add_executable(${_name}_test "${_name}_test.cpp")
    target_link_libraries(${_name}_test PUBLIC FaST)
endmacro()

add_test(bfv_add)
add_test(mat)
add_test(seal_parm)
add_test(bfv_mul)
