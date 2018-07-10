message("directory : " ${PROJECT_BINARY_DIR}/modules/SulDownloader)
add_executable(SulDownloader main.cpp)
add_dependencies(SulDownloader suldownloaderimpl)
target_include_directories(SulDownloader  PUBLIC ${PROJECT_SOURCE_DIR}/modules/SulDownloader)

set (CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${SUL_LIBRARY_DIRS} )

target_link_libraries(SulDownloader  suldownloaderimpl)


install(TARGETS SulDownloader
        DESTINATION files/base/bin)
