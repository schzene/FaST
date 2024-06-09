add_library(FaST_SCI_GC
    emp-tool.cpp
    halfgate_eva.cpp
    halfgate_gen.cpp
)

target_include_directories(FaST_SCI_GC PUBLIC
    ${PROJECT_SOURCE_DIR}/include/SCI/GC/
)

target_link_libraries(FaST_SCI_GC
    PUBLIC 
    FaST_SCI
)
