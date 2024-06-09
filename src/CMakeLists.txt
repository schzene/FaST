add_subdirectory(utils)
add_subdirectory(protocols)
add_subdirectory(SCI)

add_library(FaST
    transformer.cpp
)

target_include_directories(FaST PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(FaST PUBLIC
    FaST_Protocols
    FaST_SCI
    FaST_Utils
)