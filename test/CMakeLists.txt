include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(toy_sec_trans)
target_sources(toy_sec_trans PRIVATE toy_sec_trans.cpp)
target_link_libraries(toy_sec_trans FaST)