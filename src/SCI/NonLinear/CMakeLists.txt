add_library(FaST_SCI_NonLinear INTERFACE)

target_include_directories(FaST_SCI_NonLinear INTERFACE
	${PROJECT_SOURCE_DIR}/include/SCI/NonLinear/
)

target_link_libraries(FaST_SCI_NonLinear
	INTERFACE 
	FaST_SCI_NonLinear
)