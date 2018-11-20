
add_library(loggingobject OBJECT
        LoggingSetup.cpp
        LoggingSetup.h
        ConsoleLoggingSetup.cpp
        ConsoleLoggingSetup.h
        FileLoggingSetup.cpp
        FileLoggingSetup.h
        PluginLoggingSetup.cpp
        PluginLoggingSetup.h
        )

target_include_directories(loggingobject SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR})

add_library(logging SHARED $<TARGET_OBJECTS:loggingobject>)

SET_TARGET_PROPERTIES( logging
        PROPERTIES INSTALL_RPATH "$ORIGIN")

target_link_libraries(logging PUBLIC
        applicationconfigurationimpl
        filesystemimpl
        ${LOG4CPLUS_LIBRARY}
        )

install(TARGETS logging
        LIBRARY DESTINATION files/base/lib64)
