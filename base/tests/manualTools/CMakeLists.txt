add_executable(LoggerLimit
        LoggerLimit.cpp
        )
add_executable(FaultInjectionLogger
        FaultInjectionLogger.cpp
        )

add_executable(PickYourPoison
        PickYourPoison.cpp
        )

target_include_directories(LoggerLimit PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
target_include_directories(FaultInjectionLogger PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
target_link_libraries(LoggerLimit logging)
target_link_libraries(FaultInjectionLogger logging)

target_link_libraries(PickYourPoison pthread)

SET_TARGET_PROPERTIES(FaultInjectionLogger
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/base/lib64")