include_directories(${PROJECT_SOURCE_DIR}/src)

# cost_tets
macro(add_pro_test _name)
    add_executable(pro_${_name} "pro_${_name}_test.cpp")
    target_link_libraries(pro_${_name} PUBLIC FaST)
endmacro()

add_pro_test(attn)
add_pro_test(layer-norm)
add_pro_test(ffn)
add_pro_test(transformer)
add_pro_test(bfv_ln)

