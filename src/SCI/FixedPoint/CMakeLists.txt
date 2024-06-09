add_library(FaST_SCI_FixedPoint
    bool-data.cpp
    fixed-point.cpp
)

target_include_directories(FaST_SCI_FixedPoint PUBLIC
    ${PROJECT_SOURCE_DIR}/include/SCI/FixedPoint/
)

target_link_libraries(FaST_SCI_FixedPoint
    PUBLIC
    FaST_SCI
)