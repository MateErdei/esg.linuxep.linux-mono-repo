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

add_executable(RunHttpRequest
        RunHttpRequest.cpp)

add_executable(UseCommsComponent
        UseCommsComponent.cpp)


target_include_directories(LoggerLimit PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
target_include_directories(FaultInjectionLogger PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
target_include_directories(JailBreakerTest PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})
target_include_directories(RunHttpRequest PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})
target_include_directories(UseCommsComponent PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})

target_link_libraries(LoggerLimit logging)
target_link_libraries(FaultInjectionLogger logging)
target_link_libraries(JailBreakerTest logging commscomponentimpl)
target_link_libraries(RunHttpRequest logging commscomponentimpl)
target_link_libraries(UseCommsComponent logging commscomponentimpl)


target_link_libraries(PickYourPoison pthread)

SET_TARGET_PROPERTIES(FaultInjectionLogger
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/base/lib64")

SET_TARGET_PROPERTIES(PickYourPoison
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/base/lib64")

SET_TARGET_PROPERTIES(JailBreakerTest
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/base/lib64")

SET_TARGET_PROPERTIES(RunHttpRequest
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/base/lib64")

SET_TARGET_PROPERTIES(UseCommsComponent
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/base/lib64")
