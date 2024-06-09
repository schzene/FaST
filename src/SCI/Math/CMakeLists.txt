add_library(FaST_SCI_Math
    math-functions.cpp
)

target_include_directories(FaST_SCI_Math PUBLIC
    ${PROJECT_SOURCE_DIR}/include/SCI/Math/
)

target_link_libraries(FaST_SCI_Math
    PUBLIC 
    FaST_SCI
)


