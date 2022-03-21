
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)
add_executable(SulDownloaderTests
        TestConfigurationData.cpp
        TestWarehouseHelper.cpp
        TestWarehouseHelper.h
        TestSdds3RepositoryHelper.h
        TestSdds3RepositoryHelper.cpp
        MockWarehouseRepositoryTest.cpp
        MockWarehouseRepository.h
        MockSdds3Repository.h
        MockSdds3Wrapper.h
        SulDownloaderTests.cpp
        TestDownloadReport.cpp
        TestConnectionSelector.cpp
        TestProductSelection.cpp
        MockVersig.h
        VersigTests.cpp
        TestProxy.cpp
        TestProductUninstaller.cpp
        ConfigurationDataBase.h
        TestProductMetadata.cpp
        TestCatalogueInfo.cpp
        TestUpdateSupplementDecider.cpp
        TestSdds3Repository.cpp
        Sdds3ReplaceAndRestore.h
        Sdds3ReplaceAndRestore.cpp)

target_include_directories(SulDownloaderTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include"  ${PROTOBUF_INCLUDE_DIR} ${SDDS3LIB_INPUT} ${SOPHLIB_INCLUDE_DIR})
target_include_directories(SulDownloaderTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}"
        "${CMAKE_BINARY_DIR}/modules/SulDownloader/suldownloaderdata"
        )
target_link_libraries(SulDownloaderTests gtest gmock gtest_main)
target_link_libraries(SulDownloaderTests processimpl filesystemimpl suldownloaderimpl testhelpers threads protobufutil ${SDDS3_LIBRARY} ${SOPHLIB_LIBRARIES})
gtest_discover_tests(SulDownloaderTests)

SET_TARGET_PROPERTIES(SulDownloaderTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
