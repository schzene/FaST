include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(toy_sec_trans)
target_sources(toy_sec_trans PRIVATE toy_sec_trans.cpp)
target_link_libraries(toy_sec_trans FaST)

macro(add_test _name)
    add_executable(${_name}_test "${_name}_test.cpp")
    target_link_libraries(${_name}_test PUBLIC FaST)
endmacro()

add_test(attn)
add_test(io)