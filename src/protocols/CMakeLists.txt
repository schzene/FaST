add_library(FaST_Protocols
    attention.cpp
    layer-norm1.cpp
    layer-norm2.cpp
)

target_link_libraries(FaST_Protocols
    PUBLIC
    FaST_Utils
)