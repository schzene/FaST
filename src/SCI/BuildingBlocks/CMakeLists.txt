add_library(FaST_SCI_BuildingBlocks
    aux-protocols.cpp
    linear-ot.cpp
    truncation.cpp
    value-extension.cpp
)

target_include_directories(FaST_SCI_BuildingBlocks PUBLIC
    ${PROJECT_SOURCE_DIR}/include/SCI/BuildingBlocks/
)

target_link_libraries(FaST_SCI_BuildingBlocks
    PUBLIC
    OpenMP::OpenMP_CXX
    Eigen3::Eigen
    
    FaST_SCI
)