
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)
add_executable(SulDownloaderTests
        TestConfigurationData.cpp
        TestSdds3RepositoryHelper.h
        TestSdds3RepositoryHelper.cpp
        TestSDDS3Utils.cpp
        MockSdds3Repository.h
        MockSdds3Wrapper.h
        MockSignatureVerifierWrapper.h
        MockSusRequester.h
        SulDownloaderSdds3Tests.cpp
        TestDownloadReportBuilder.cpp
        TestConnectionSelector.cpp
        MockVersig.h
        VersigTests.cpp
        TestProductUninstaller.cpp
        ../Common/Helpers/ConfigurationDataBase.h
        TestProductMetadata.cpp
        TestSdds3Repository.cpp
        Sdds3ReplaceAndRestore.h
        Sdds3ReplaceAndRestore.cpp
        TestSusRequester.cpp
        TestInstalledFeatures.cpp
        TestSulDownloaderUtils.cpp
        SulDownloaderESMTests.cpp
        SuldownloaderSDDS3Helper.h)

target_include_directories(SulDownloaderTests SYSTEM BEFORE PUBLIC
        "${GTEST_INCLUDE}"
        "${GMOCK_INCLUDE}"
        ${PROTOBUF_INCLUDE_DIR}
        ${GSL_INCLUDE}
        ${SOPHLIB_INCLUDE}
        ${CMAKE_BINARY_DIR}/modules/Common/Policy
        ${NLOHMANN_JSON_INCLUDE_DIR}
        )

target_include_directories(SulDownloaderTests BEFORE PUBLIC
        "${CMAKE_BINARY_DIR}/modules"
        "${PROJECT_SOURCE_DIR}"
        "${CMAKE_BINARY_DIR}/modules/SulDownloader/suldownloaderdata"
        )
target_link_libraries(SulDownloaderTests ${GTEST_MAIN_LIBRARY})
target_link_libraries(SulDownloaderTests processimpl filesystemimpl suldownloaderimpl testhelpers threads protobufutil suldownloaderdata policy)
gtest_discover_tests(SulDownloaderTests)

SET_TARGET_PROPERTIES(SulDownloaderTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
