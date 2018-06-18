message("directory : " ${PROJECT_BINARY_DIR}/modules/SulDownloader)
add_executable(SulDownloader main.cpp)
add_dependencies(SulDownloader suldownloaderimpl)
target_include_directories(SulDownloader  PUBLIC ${PROJECT_SOURCE_DIR}/modules/SulDownloader)

#/redist/binaries/linux10/input/SUL
set( REDIST /redist )
set( BINARIES_DIR ${REDIST}/binaries )
set( INPUT ${BINARIES_DIR}/linux10/input )
set( SUL_INPUT ${INPUT}/SUL )
set( BOOST_INPUT ${INPUT}/boost )
set( SUL_LIBRARY_DIR ${SUL_INPUT}/lib64 )
set( SUL_LIBRARY_DIRS ${SUL_LIBRARY_DIR}:${BOOST_INPUT}/lib64 )

set (CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${SUL_LIBRARY_DIRS} )

target_link_libraries(SulDownloader  suldownloaderimpl)