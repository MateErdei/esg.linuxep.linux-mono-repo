add_executable(LoggerLimit LoggerLimit.cpp)
add_executable(FaultInjectionLogger FaultInjectionLogger.cpp)
add_executable(PickYourPoison PickYourPoison.cpp)
add_executable(ignoreSignals ignoreSignals.cpp)
add_executable(EventPubSub EventPubSub.cpp)
add_executable(SDDS3DownloaderTest SDDS3DownloaderTest.cpp)

target_include_directories(LoggerLimit PUBLIC ${LOG4CPLUS_INCLUDE_DIR})
target_include_directories(FaultInjectionLogger PUBLIC ${LOG4CPLUS_INCLUDE_DIR})
target_include_directories(EventPubSub PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(SDDS3DownloaderTest PUBLIC ${SOPHLIB_INCLUDE_DIR} ${SDDS3LIB_INPUT} ${CURL_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/thirdparty/nlohmann-json ${GSL_INCLUDE})
target_compile_options(SDDS3DownloaderTest PUBLIC -Wno-unused-value -Wno-unused-parameter -Wno-maybe-uninitialized -Wno-stringop-overflow)

target_link_libraries(LoggerLimit logging)
target_link_libraries(FaultInjectionLogger logging)
target_link_libraries(EventPubSub zmqwrapperapiimpl)
target_link_libraries(SDDS3DownloaderTest
        filesystemimpl
        -L${SDDS3LIB_INPUT} -lsdds3 -ldelta -lpugixml -lutils -lsdds3crypto
        -lminizip
        stdc++fs
        ${SOPHLIB_LIBRARIES}
        ${OPENSSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        -Wno-unused-parameter -Wno-maybe-uninitialized -Wno-stringop-overflow
        )
target_link_libraries(PickYourPoison pthread)

SET_TARGET_PROPERTIES(FaultInjectionLogger PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")

SET_TARGET_PROPERTIES(PickYourPoison PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")


SET_TARGET_PROPERTIES(EventPubSub PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")

SET_TARGET_PROPERTIES(SDDS3DownloaderTest PROPERTIES
        BUILD_RPATH "/opt/sophos-spl/base/lib64"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")


INSTALL(TARGETS
        LoggerLimit
        FaultInjectionLogger
        PickYourPoison
        ignoreSignals
        EventPubSub
        SDDS3DownloaderTest
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)

add_executable(diskSpaceUtil DiskSpaceUtil.cpp)

target_include_directories(diskSpaceUtil PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})

target_link_libraries(diskSpaceUtil logging filesystemimpl)

SET_TARGET_PROPERTIES(diskSpaceUtil PROPERTIES
        BUILD_RPATH "/opt/sophos-spl/base/lib64"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")


INSTALL(TARGETS
        diskSpaceUtil
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)