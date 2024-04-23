add_library(FaST_Protocols
    attention.cpp
)

target_link_libraries(FaST_Protocols
    PUBLIC
    FaST_Utils
)
