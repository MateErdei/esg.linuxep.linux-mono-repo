include(GoogleTest)
add_executable(SulDownloaderTests
        TestConfigurationData.cpp
        TestWarehouseHelper.cpp
        TestWarehouseHelper.h
        MockWarehouseRepositoryTest.cpp
        MockWarehouseRepository.h
        SulDownloaderTests.cpp
        TestDownloadReport.cpp
        TestConnectionSelector.cpp
        TestProductSelection.cpp
        MockVersig.h VersigTests.cpp)

target_include_directories(SulDownloaderTests PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_include_directories(SulDownloaderTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(SulDownloaderTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/modules/SulDownloader")
target_link_libraries(SulDownloaderTests gtest gmock gtest_main)
target_link_libraries(SulDownloaderTests processimpl filesystemimpl suldownloaderimpl testhelpers threads ${Protobuf_LIBRARIES})
gtest_discover_tests(SulDownloaderTests)

#/redist/binaries/linux10/input/SUL
set(CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${SUL_LIBRARY_DIRS})
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

set(SUL_LIBRARY -L${SUL_LIBRARY_DIR}  -lSUL)