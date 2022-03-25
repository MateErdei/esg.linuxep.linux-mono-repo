add_executable(LoggerLimit
        LoggerLimit.cpp
        )
add_executable(FaultInjectionLogger
        FaultInjectionLogger.cpp
        )

add_executable(PickYourPoison
        PickYourPoison.cpp
        )
add_executable(JailBreakerTest
        JailBreakerTest.cpp
)
add_executable(ignoreSignals
        ignoreSignals.cpp
        )
add_executable(RunHttpRequest
        RunHttpRequest.cpp)

add_executable(UseCommsComponent
        UseCommsComponent.cpp)

add_executable(EventPubSub
        EventPubSub.cpp)


target_include_directories(LoggerLimit PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
target_include_directories(FaultInjectionLogger PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
target_include_directories(JailBreakerTest PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})
target_include_directories(RunHttpRequest PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})
target_include_directories(UseCommsComponent PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})
target_include_directories(EventPubSub PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(LoggerLimit logging)
target_link_libraries(FaultInjectionLogger logging)
target_link_libraries(JailBreakerTest logging commscomponentimpl)
target_link_libraries(RunHttpRequest logging commscomponentimpl)
target_link_libraries(UseCommsComponent logging commscomponentimpl)
target_link_libraries(EventPubSub zmqwrapperapiimpl)
target_link_libraries(PickYourPoison pthread)

# TODO once we update to modern cmake version
#target_link_options(LoggerLimit PRIVATE -static-libstdc++)
#target_link_options(FaultInjectionLogger PRIVATE -static-libstdc++)
#target_link_options(JailBreakerTest PRIVATE -static-libstdc++)
#target_link_options(RunHttpRequest PRIVATE -static-libstdc++)
#target_link_options(UseCommsComponent PRIVATE -static-libstdc++)
#target_link_options(EventPubSub PRIVATE -static-libstdc++)

SET_TARGET_PROPERTIES(FaultInjectionLogger PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")

SET_TARGET_PROPERTIES(PickYourPoison PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")

SET_TARGET_PROPERTIES(JailBreakerTest PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")

SET_TARGET_PROPERTIES(RunHttpRequest PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")

INSTALL(TARGETS RunHttpRequest
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)

SET_TARGET_PROPERTIES(UseCommsComponent PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")

SET_TARGET_PROPERTIES(EventPubSub PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")
