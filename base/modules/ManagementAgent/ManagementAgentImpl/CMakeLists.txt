add_library(managementagentimpl SHARED
        ManagementAgentMain.h
        ManagementAgentMain.cpp
        ../LoggerImpl/Logger.h
        )
target_include_directories(managementagentimpl PUBLIC
        ../../Common
        ${LOG4CPLUS_INCLUDE_DIR}
        ${AUTOVERSION_INCLUDE}
)
target_link_libraries(managementagentimpl PUBLIC
        pluginapiimpl
        applicationconfigurationimpl
        plugincommunicationimpl
        policyreceiverimpl
        directorywatcherimpl
        statusreceiverimpl
        eventreceiverimpl
        taskqueueimpl
        mcsrouterplugincommunicationimpl
        managementagentloggerimpl
        telemetryhelperimpl
        ThreatHealthReceiverImpl
        )

SET_TARGET_PROPERTIES( managementagentimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS managementagentimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)