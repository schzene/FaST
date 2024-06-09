add_library(FaST_SCI_OT
    INTERFACE
)

target_include_directories(FaST_SCI_OT INTERFACE
    ${PROJECT_SOURCE_DIR}/include/SCI/OT/
)

target_link_libraries(FaST_SCI_OT
    INTERFACE
    FaST_SCI_Utils
)