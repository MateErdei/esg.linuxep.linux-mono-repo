add_library(eventreceiverimpl SHARED
        EventReceiverImpl.cpp EventReceiverImpl.h
        ../EventReceiver/IEventReceiver.h
        EventTask.cpp
        EventTask.h
        EventUtils.cpp
        EventUtils.h
        IOutbreakModeController.h
        OutbreakModeController.cpp
        OutbreakModeController.h
        Event.cpp Event.h)

target_include_directories(eventreceiverimpl PUBLIC  ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})
target_link_libraries(eventreceiverimpl PUBLIC
        applicationconfigurationimpl
        managementagentloggerimpl
        taskqueueimpl
        xmlutilities
        )

SET_TARGET_PROPERTIES( eventreceiverimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS eventreceiverimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
