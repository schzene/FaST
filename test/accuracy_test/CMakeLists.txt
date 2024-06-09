include_directories(${PROJECT_SOURCE_DIR}/src)

# acc test
macro(add_acc_test _name)
    add_executable(acc_${_name} "acc_${_name}_test.cpp")
    target_link_libraries(acc_${_name} PUBLIC FaST)
endmacro()

add_acc_test(attn)
add_acc_test(layer-norm)
add_acc_test(ffn)

# bfv_acc test
macro(add_bfv_acc_test _name)
    add_executable(bfv_acc_${_name} "bfv_acc_${_name}_test.cpp")
    target_link_libraries(bfv_acc_${_name} PUBLIC FaST)
endmacro()

# add_bfv_acc_test(layer-norm)