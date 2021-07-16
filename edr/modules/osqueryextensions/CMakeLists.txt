sophos_add_library(OsqueryExtensions SHARED
        ThreatTypes.h
        IServiceExtension.h
        LoggerExtension.h
        LoggerExtension.cpp
        Logger.h
        Logger.cpp
        ResultsSender.h
        ResultsSender.cpp
        SophosServerTable.h
        SophosServerTable.cpp
        SophosAVDetectionTable.h
        SophosAVDetectionTable.cpp
        SophosExtension.h
        SophosExtension.cpp
        ${SQLITE_INCLUDE_DIR}/sqlite3.c
        EXTRA_PROJECTS
            eventjournalwrapperimpl

        EXTRA_LIBS
            ${JOURNAL_LIBRARY}
            ${log4cpluslib}
            ${protobuflib}
            ${OSQUERYSDK_LIBRARY}
            ${OSQUERY_GEN_LIBRARY}
            ${LOGGER_PLUGIN_LIBRARY}
            ${THRIFT_LIBRARY}
            ${JSON_CPP_LIBRARY}
            ${GFLAG_LIBRARY}
            ${GLOG_LIBRARY}
            ${THRIFT_GEN_LIBRARY}
            pthread

        EXTRA_INCLUDES ${OSQUERYSDK}/include ${SQLITE_INCLUDE_DIR} ${CMAKE_BINARY_DIR}  ${INPUT}/jsoncpp/include ${BOOST_INCLUDE_DIR} ${JOURNAL_INCLUDE_DIR}
        )