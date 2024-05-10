include_directories(${PROJECT_SOURCE_DIR}/include)
add_subdirectory(utils)
add_subdirectory(protocols)
add_library(FaST
    transformer.cpp
)

target_link_libraries(FaST PUBLIC
    FaST_Utils
    FaST_Protocols
)

target_include_directories(FaST
    INTERFACE
    ${PROJECT_SOURCE_DIR}/include
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:./>
)

install(DIRECTORY
  FaST
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp")