add_subdirectory(utils)
add_library(FaST INTERFACE)


target_link_libraries(FaST
    INTERFACE
    FaST_Utils
)

target_include_directories(FaST
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:./>
)

install(DIRECTORY
  FaST
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp")