find_package(Eigen3 3.3 QUIET NO_MODULE PATHS "${PROJECT_SOURCE_DIR}/extern/eigen/build" NO_DEFAULT_PATH)
if (NOT Eigen3_FOUND)
    message(STATUS "Eigen 3.3 was not found: clone and install Eigen3 locally")
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/eigen/CMakeLists.txt")
        find_package(Git REQUIRED)
        message(STATUS "initialize Git submodule: extern/eigen")
        execute_process(COMMAND git submodule update --init --recursive extern/eigen
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory build
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/extern/eigen/")
    execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${PROJECT_SOURCE_DIR}/build ..
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/extern/eigen/build")
    find_package(Eigen3 3.3 REQUIRED NO_MODULE PATHS "${PROJECT_SOURCE_DIR}/extern/eigen/build" NO_DEFAULT_PATH)
endif()

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