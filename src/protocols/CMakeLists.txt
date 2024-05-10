add_library(FaST_Protocols
    attention.cpp
    ffn.cpp
    layer-norm1.cpp
    layer-norm2.cpp
)

target_include_directories(FaST_Protocols
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include/protocols
)

target_link_libraries(FaST_Protocols
    PUBLIC
    FaST_Utils
)