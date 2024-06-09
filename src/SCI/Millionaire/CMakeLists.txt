add_library(FaST_SCI_Millionaire
    INTERFACE
)

target_include_directories(FaST_SCI_Millionaire INTERFACE
    ${PROJECT_SOURCE_DIR}/include/SCI/Millionaire/
)

target_link_libraries(FaST_SCI_Millionaire
    INTERFACE
    FaST_SCI_OT    
)