
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)
add_executable(SulDownloaderTests
        TestConfigurationData.cpp
        TestSdds3RepositoryHelper.h
        TestSdds3RepositoryHelper.cpp
        TestSDDS3Utils.cpp
        MockSdds3Repository.h
        MockSdds3Wrapper.h
        MockSusRequester.h
        SulDownloaderSdds3Tests.cpp
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
        Sdds3ReplaceAndRestore.cpp
        TestSusRequester.cpp
        TestInstalledFeatures.cpp
        TestSulDownloaderUtils.cpp)

target_include_directories(SulDownloaderTests SYSTEM BEFORE PUBLIC
        "${GTEST_INCLUDE}"
        "${GMOCK_INCLUDE}"
        ${PROTOBUF_INCLUDE_DIR}
        ${GSL_INCLUDE}
        )
target_include_directories(SulDownloaderTests BEFORE PUBLIC "${PROJECT_SOURCE_DIR}"
        "${CMAKE_BINARY_DIR}/modules/SulDownloader/suldownloaderdata"
        )
target_link_libraries(SulDownloaderTests  ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
target_link_libraries(SulDownloaderTests processimpl filesystemimpl suldownloaderimpl testhelpers threads protobufutil)
gtest_discover_tests(SulDownloaderTests)

SET_TARGET_PROPERTIES(SulDownloaderTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
