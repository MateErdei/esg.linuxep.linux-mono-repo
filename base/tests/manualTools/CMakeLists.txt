add_executable(LoggerLimit LoggerLimit.cpp)
add_executable(FaultInjectionLogger FaultInjectionLogger.cpp)
add_executable(PickYourPoison PickYourPoison.cpp)
add_executable(ignoreSignals ignoreSignals.cpp)
add_executable(EventPubSub EventPubSub.cpp)

target_include_directories(LoggerLimit PUBLIC ${LOG4CPLUS_INCLUDE_DIR})
target_include_directories(FaultInjectionLogger PUBLIC ${LOG4CPLUS_INCLUDE_DIR})
target_include_directories(EventPubSub PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(LoggerLimit logging)
target_link_libraries(FaultInjectionLogger logging)
target_link_libraries(EventPubSub zmqwrapperapiimpl)
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

INSTALL(TARGETS
        LoggerLimit
        FaultInjectionLogger
        PickYourPoison
        ignoreSignals
        EventPubSub
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)


add_executable(diskSpaceUtil DiskSpaceUtil.cpp)

target_include_directories(diskSpaceUtil PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(diskSpaceUtil filesystemimpl)

SET_TARGET_PROPERTIES(diskSpaceUtil PROPERTIES
        BUILD_RPATH "/opt/sophos-spl/base/lib64"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")

INSTALL(TARGETS
        diskSpaceUtil
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)