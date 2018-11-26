add_library(managementagentimpl SHARED
        ManagementAgentMain.h
        ManagementAgentMain.cpp
        ../LoggerImpl/Logger.h
        )
target_include_directories(managementagentimpl PUBLIC  ../../Common ${LOG4CPLUS_INCLUDE_DIR})
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
        )

SET_TARGET_PROPERTIES( managementagentimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS managementagentimpl
        LIBRARY DESTINATION files/base/lib64)