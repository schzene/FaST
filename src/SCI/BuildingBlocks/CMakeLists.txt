find_package(Eigen3 3.3 QUIET NO_MODULE PATHS "${PROJECT_SOURCE_DIR}/build/eigen" NO_DEFAULT_PATH)
if (NOT Eigen3_FOUND)
    message(STATUS "Eigen 3.3 was not found: clone and install Eigen3 locally")
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/eigen/CMakeLists.txt")
        find_package(Git REQUIRED)
        message(STATUS "initialize Git submodule: extern/eigen")
        execute_process(COMMAND git submodule update --init --recursive extern/eigen
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory eigen
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/build")
    execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${PROJECT_SOURCE_DIR}/build/eigen ${PROJECT_SOURCE_DIR}/extern/eigen
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/build/eigen")
    execute_process(COMMAND ${CMAKE_COMMAND} --build . --target install -j 16
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/build")
    find_package(Eigen3 3.3 REQUIRED NO_MODULE PATHS "${PROJECT_SOURCE_DIR}/build/eigen" NO_DEFAULT_PATH)
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