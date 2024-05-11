include_directories(${PROJECT_SOURCE_DIR}/src)

macro(add_acc_test _name)
    add_executable(acc_${_name} "acc_${_name}_test.cpp")
    target_link_libraries(acc_${_name} PUBLIC FaST)
endmacro()

add_acc_test(attn)
add_acc_test(layer-norm1)
add_acc_test(ffn)


macro(add_pro_test _name)
    add_executable(pro_${_name} "pro_${_name}_test.cpp")
    target_link_libraries(pro_${_name} PUBLIC FaST)
endmacro()

add_pro_test(attn)
add_pro_test(layer-norm1)
add_pro_test(ffn)
add_pro_test(transformer)

add_executable(RFCP_matmul_test "RFCP_matmul_test.cpp")
target_link_libraries(RFCP_matmul_test PUBLIC FaST)


find_package(OpenMP REQUIRED)
add_executable(omp_test "opm_test.cpp")
target_link_libraries(omp_test 
PUBLIC 
FaST 
OpenMP::OpenMP_CXX )