find_package(OpenMP REQUIRED)
find_package(SEAL 4.1.1 EXACT QUIET PATHS "${PROJECT_SOURCE_DIR}/build/" NO_DEFAULT_PATH)
if (NOT SEAL_FOUND)
    message(STATUS "SEAL 4.1.1 was not found: clone and install SEAL locally")
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/SEAL/CMakeLists.txt")
        find_package(Git REQUIRED)
        message(STATUS "initialize Git submodule: extern/SEAL")
        execute_process(COMMAND git submodule update --init --recursive extern/SEAL
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
        execute_process(COMMAND git apply "${PROJECT_SOURCE_DIR}/cmake/seal.patch"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/extern/SEAL")
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${PROJECT_SOURCE_DIR}/build .
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/extern/SEAL/")
    execute_process(COMMAND ${CMAKE_COMMAND} --build . --target install -j8
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/extern/SEAL/")
    find_package(SEAL 4.1.1 EXACT REQUIRED PATHS "${PROJECT_SOURCE_DIR}/build/" NO_DEFAULT_PATH)
endif()

add_library(FaST_Utils
    # conversion.cpp
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