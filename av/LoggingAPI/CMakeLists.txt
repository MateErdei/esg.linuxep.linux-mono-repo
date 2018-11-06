
add_library(loggingapi SHARED
        LoggingSetup.cpp
        LoggingSetup.h
        ConsoleLoggingSetup.cpp
        ConsoleLoggingSetup.h
        FileLoggingSetup.cpp
        FileLoggingSetup.h
        )

target_link_libraries(loggingapi PUBLIC
        ${pluginapilib}
        ${log4cpluslib}
        )
target_include_directories(loggingapi SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR})

SET_TARGET_PROPERTIES( loggingapi
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS loggingapi
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)
