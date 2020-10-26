sophos_add_library(OsqueryExtensions SHARED
        IServiceExtension.h
        LoggerExtension.h
        LoggerExtension.cpp
        Logger.h
        Logger.cpp
        ResultsSender.h
        ResultsSender.cpp
        ${SQLITE_INCLUDE_DIR}/sqlite3.c
        EXTRA_LIBS ${log4cpluslib} ${protobuflib} ${NEW_OSQUERY_LIBRARY} ${LOGGER_PLUGIN_LIBRARY} ${THRIFT_LIBRARY}  ${JSON_CPP_LIBRARY} ${GFLAG_LIBRARY} ${GLOG_LIBRARY} ${THRIFT_GEN_LIBRARY}
        EXTRA_INCLUDES ${SSPLOSQUERYCOMPONENTSPATH}/include ${SQLITE_INCLUDE_DIR} ${CMAKE_BINARY_DIR}
        )