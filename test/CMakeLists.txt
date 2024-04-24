include_directories(${PROJECT_SOURCE_DIR}/src)

macro(add_toy _name)
    add_executable(toy_${_name})
    target_sources(toy_${_name} PRIVATE toy_${_name}.cpp)
    target_link_libraries(toy_${_name} FaST)
endmacro()

add_toy(attn)


macro(add_test _name)
    add_executable(${_name}_test "${_name}_test.cpp")
    target_link_libraries(${_name}_test PUBLIC FaST)
endmacro()

add_test(attn)