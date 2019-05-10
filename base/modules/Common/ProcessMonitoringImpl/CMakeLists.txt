add_library(processmonitoringimplobject OBJECT
        SignalHandler.cpp
        SignalHandler.h
        ProcessProxy.cpp
        ProcessProxy.h
        Logger.cpp
        Logger.h
        )
target_include_directories(processmonitoringimplobject PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
add_library(processmonitoringimpl SHARED $<TARGET_OBJECTS:processmonitoringimplobject> )
target_link_libraries(processmonitoringimpl PUBLIC ${ZeroMQ_LIBRARY} processimpl)

SET_TARGET_PROPERTIES( processmonitoringimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS processmonitoringimpl
        LIBRARY DESTINATION files/base/lib64)