add_subdirectory(tools)
add_library(FastST INTERFACE)


target_link_libraries(FastST
    INTERFACE
    FastST_Tools
)

target_include_directories(FastST
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:./>
)

install(DIRECTORY
  FastST
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp")