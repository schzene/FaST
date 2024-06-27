include_directories(${CMAKE_CURRENT_LIST_DIR})

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)

add_subdirectory(BuildingBlocks)
add_subdirectory(FixedPoint)
add_subdirectory(GC)
add_subdirectory(Math)
add_subdirectory(Millionaire)
add_subdirectory(NonLinear)
add_subdirectory(OT)
add_subdirectory(Utils)

add_library(FaST_SCI
    INTERFACE
)

target_link_libraries(FaST_SCI
    INTERFACE
    FaST
    FaST_SCI_BuildingBlocks
    FaST_SCI_FixedPoint
    FaST_SCI_GC
    FaST_SCI_Math
    FaST_SCI_Millionaire
    FaST_SCI_NonLinear
    FaST_SCI_OT
    FaST_SCI_Utils
)

target_include_directories(FaST_SCI
    INTERFACE
    ${PROJECT_SOURCE_DIR}/include/SCI/
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:./>
)





