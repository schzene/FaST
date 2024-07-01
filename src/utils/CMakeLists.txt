find_package(OpenMP REQUIRED)

add_library(FaST_Utils
    conversion.cpp
    he-bfv.cpp
    he-tools.cpp
    mat-tools.cpp
)

target_include_directories(FaST_Utils PRIVATE
    ${PROJECT_SOURCE_DIR}/include/utils
)

target_link_libraries(FaST_Utils PUBLIC
    OpenMP::OpenMP_CXX
    SEAL::seal
    FaST
    FaST_SCI
)