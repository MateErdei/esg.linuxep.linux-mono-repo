add_library(watchdogservice SHARED
        ../WatchdogRequest/IWatchdogRequest.h
        ../WatchdogRequest/WatchdogServiceException.h
        WatchdogRequestImpl.cpp
        Logger.h
        Logger.cpp
        )

target_include_directories(watchdogservice SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})

target_link_libraries(watchdogservice PUBLIC
        logging
        processmonitoringimpl
        pluginapiimpl
        plugincommunicationimplcommon
        telemetryhelperimpl
        )

SET_TARGET_PROPERTIES(watchdogservice
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS watchdogservice
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)