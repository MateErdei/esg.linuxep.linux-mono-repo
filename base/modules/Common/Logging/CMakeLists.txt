
add_library(logging SHARED
        LoggingSetup.cpp
        LoggingSetup.h
        ConsoleLoggingSetup.cpp
        ConsoleLoggingSetup.h
        FileLoggingSetup.cpp
        FileLoggingSetup.h
        )

target_link_libraries(logging PUBLIC ${LOG4CPLUS_LIBRARY} applicationconfigurationimpl)
target_include_directories(logging SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR})

SET_TARGET_PROPERTIES( logging
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS logging
        LIBRARY DESTINATION files/base/lib64)
