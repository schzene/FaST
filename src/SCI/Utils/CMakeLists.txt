set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(cmake/source_of_randomness.cmake)

find_package(OpenSSL REQUIRED)
FIND_PACKAGE(GMP REQUIRED)

add_library(FaST_SCI_Utils INTERFACE)

target_include_directories(FaST_SCI_Utils INTERFACE
    ${PROJECT_SOURCE_DIR}/include/SCI/Utils/
    ${OPENSSL_INCLUDE_DIR} ${GMP_INCLUDE_DIR}
)

target_compile_options(FaST_SCI_Utils
    INTERFACE
    "-pthread;-maes;-msse4.1;-mavx;-mavx2;-faligned-new;-std=c++17;-O3;;-Wno-deprecated-declarations;")
    # "-pthread;-maes;-msse4.1;-mavx;-mavx2;-faligned-new;-std=c++17;-O3;-Wno-parentheses-equality;-Wno-deprecated-declarations;-Wno-macro-redefined")
#     "-pthread;-Wall;-maes;-msse4.1;-mavx;-mavx2;-faligned-new;-std=c++17;-w;-g;-ggdb;-O0")


if(USE_RANDOM_DEVICE)
    target_compile_definitions(FaST_SCI_Utils INTERFACE EMP_USE_RANDOM_DEVICE=1)
else(USE_RANDOM_DEVICE)
    target_compile_options(FaST_SCI_Utils INTERFACE "-mrdseed")
endif(USE_RANDOM_DEVICE)

target_link_libraries(FaST_SCI_Utils
    INTERFACE ${OPENSSL_LIBRARIES} ${GMP_LIBRARIES}
)