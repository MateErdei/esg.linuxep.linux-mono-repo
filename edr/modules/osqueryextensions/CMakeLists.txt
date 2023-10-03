sophos_add_library(OsqueryExtensions SHARED
        ThreatTypes.h
        IServiceExtension.h
        LoggerExtension.h
        LoggerExtension.cpp
        Logger.h
        Logger.cpp
        ResultsSender.h
        ResultsSender.cpp
        BatchTimer.h
        BatchTimer.cpp
        GrepTable.cpp
        GrepTable.h
        HexToIntTable.cpp
        HexToIntTable.h
        SophosServerTable.h
        SophosAVDetectionTableGenerator.h
        SophosAVDetectionTableGenerator.cpp
        SophosServerTable.cpp
        SophosAVDetectionTable.h
        SophosAVDetectionTable.cpp
        SophosExtension.h
        SophosExtension.cpp
        ConstraintHelpers.h
        ConstraintHelpers.cpp
        EXTRA_PROJECTS
            eventjournalwrapperimpl

        EXTRA_LIBS
            ${JOURNAL_LIBRARY}
            ${log4cpluslib}
            ${protobuflib}
            ${JSON_CPP_LIBRARY}
            ${GFLAG_LIBRARY}
            ${GLOG_LIBRARY}
            ${BOOST_THREAD_LIBRARY}
            pthread
            logger_plugin_library
            osquerysdk_library
            sqlite_library

        EXTRA_INCLUDES
                ${CMAKE_BINARY_DIR}
                ${INPUT}/jsoncpp/include
                ${BOOST_INCLUDE_DIR}
                ${JOURNAL_INCLUDE_DIR}
                ${LIVEQUERY_INCLUDE}
                ${CMAKE_SOURCE_DIR}/modules/EdrCommon
                ${NLOHMANN_JSON_INCLUDE_DIR}
        )

set(TABLE_PLUGIN_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/SophosServerTable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SophosAVDetectionTable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GrepTable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HexToIntTable.h
)

add_custom_target(schema DEPENDS ${CMAKE_BINARY_DIR}/schema/schema.json)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/schema/schema.json
                   COMMAND python3 ${SCHEMA_BUILDER_DIR}/schema_builder.py
                        --headers ${TABLE_PLUGIN_HDRS}
                        --output ${CMAKE_BINARY_DIR}/schema
)

add_dependencies(OsqueryExtensions schema)
