find_package(OpenMP)
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


macro(add_test _name)
    add_executable(${_name}_test "${_name}_test.cpp")
    target_link_libraries(${_name}_test PUBLIC FaST)
endmacro()

add_test(RFCP_matmul)
add_test(mat)
add_test(BOLT_CP_matmul)