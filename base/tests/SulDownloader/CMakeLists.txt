add_executable(SulDownloaderTests TestProductMetadata.cpp TestConfigurationData.cpp)
target_include_directories(SulDownloaderTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(SulDownloaderTests BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(SulDownloaderTests gtest gtest_main)
target_link_libraries(SulDownloaderTests processimpl filesystemimpl suldownloaderimpl threads)
add_test(NAME SulDownloaderTests COMMAND SulDownloaderTests)

#/redist/binaries/linux10/input/SUL
set( REDIST /redist )
set( BINARIES_DIR ${REDIST}/binaries )
set( INPUT ${BINARIES_DIR}/linux10/input )
set( SUL_INPUT ${INPUT}/SUL )
set( BOOST_INPUT ${INPUT}/boost )
set( SUL_LIBRARY_DIR ${SUL_INPUT}/lib64 )
set( SUL_LIBRARY_DIRS ${SUL_LIBRARY_DIR}:${BOOST_INPUT}/lib64)

set (CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${SUL_LIBRARY_DIRS} )
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

set( SUL_LIBRARY -L${SUL_LIBRARY_DIR}  -lSUL)