add_library(ThreatHealthReceiverImpl SHARED
        ThreatHealthReceiverImpl.h
        ThreatHealthReceiverImpl.cpp
        ThreatHealthTask.h
        ThreatHealthTask.cpp
        )
target_include_directories(ThreatHealthReceiverImpl PUBLIC ../../Common ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})
target_link_libraries(ThreatHealthReceiverImpl PUBLIC applicationconfigurationimpl managementagentloggerimpl taskqueueimpl)

SET_TARGET_PROPERTIES(ThreatHealthReceiverImpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS ThreatHealthReceiverImpl
        LIBRARY DESTINATION base/files/base/lib64)
