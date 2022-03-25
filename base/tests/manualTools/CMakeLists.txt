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

add_executable(SDDS3DownloaderTest
        SDDS3DownloaderTest.cpp)


target_include_directories(LoggerLimit PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
target_include_directories(FaultInjectionLogger PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
target_include_directories(JailBreakerTest PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})
target_include_directories(RunHttpRequest PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})
target_include_directories(UseCommsComponent PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})
target_include_directories(EventPubSub PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(SDDS3DownloaderTest PUBLIC ${SOPHLIB_INCLUDE_DIR} ${SDDS3LIB_INPUT} ${CURL_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/thirdparty/nlohmann-json)
target_compile_options(SDDS3DownloaderTest PUBLIC -Wno-unused-value -Wno-unused-parameter -Wno-maybe-uninitialized -Wno-stringop-overflow)

target_link_libraries(LoggerLimit logging)
target_link_libraries(FaultInjectionLogger logging)
target_link_libraries(JailBreakerTest logging commscomponentimpl)
target_link_libraries(RunHttpRequest logging commscomponentimpl)
target_link_libraries(UseCommsComponent logging commscomponentimpl)
target_link_libraries(EventPubSub zmqwrapperapiimpl)
target_link_libraries(SDDS3DownloaderTest filesystemimpl -L${SDDS3LIB_INPUT} -lsdds3 -ldelta -lpugixml -lutils -lsdds3crypto -lminizip stdc++fs ${SOPHLIB_LIBRARIES} -Wno-unused-parameter -Wno-maybe-uninitialized -Wno-stringop-overflow)


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

SET_TARGET_PROPERTIES(EventPubSub
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/base/lib64")

SET_TARGET_PROPERTIES(SDDS3DownloaderTest
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/base/lib64")
