
add_library(loggingobject OBJECT
        LoggingSetup.cpp
        LoggingSetup.h
        ConsoleLoggingSetup.cpp
        ConsoleLoggingSetup.h
        ConsoleFileLoggingSetup.h
        ConsoleFileLoggingSetup.cpp
        FileLoggingSetup.cpp
        FileLoggingSetup.h
        PluginLoggingSetup.cpp
        PluginLoggingSetup.h
        LoggerConfig.h
        LoggerConfig.cpp
        )

target_include_directories(loggingobject SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})

add_library(logging SHARED $<TARGET_OBJECTS:loggingobject>)

SET_TARGET_PROPERTIES(logging
        PROPERTIES INSTALL_RPATH "$ORIGIN")

target_link_libraries(logging PUBLIC
        applicationconfigurationimpl
        filesystemimpl
        ${LOG4CPLUS_LIBRARY}
        pthread
        )

install(TARGETS logging
        LIBRARY DESTINATION files/base/lib64)
