
add_executable(loggerconfigtests
        loggerconfigtests.cpp
        $<TARGET_OBJECTS:loggingobject>
        $<TARGET_OBJECTS:filesystemimplobject>
        $<TARGET_OBJECTS:applicationconfigurationimplobject>)

target_include_directories(loggerconfigtests PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})

target_link_libraries(loggerconfigtests PUBLIC
         ${LOG4CPLUS_LIBRARY}
        )

SET_TARGET_PROPERTIES( loggerconfigtests
        PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

set( ALCParseDir ${CMAKE_SOURCE_DIR}/modules/UpdateSchedulerImpl/configModule)
add_executable(parsealcpolicytests
        parsealcpolicytests.cpp
        ${ALCParseDir}/UpdatePolicyTranslator.cpp
        ${ALCParseDir}/UpdatePolicyTranslator.h
        ${ALCParseDir}/../../UpdateScheduler/ScheduledUpdate.cpp
        ${ALCParseDir}/../../UpdateScheduler/ScheduledUpdate.h
        ${ALCParseDir}/../Logger.cpp
        ${ALCParseDir}/../Logger.h
        )
target_link_libraries(parsealcpolicytests PRIVATE
        ${LOG4CPLUS_LIBRARY}
        xmlutilities
        utilityimpl
        osutilitiesimpl
        suldownloaderdata
        )

SET_TARGET_PROPERTIES( parsealcpolicytests
        PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

target_include_directories(parsealcpolicytests PUBLIC ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/modules/UpdateSchedulerImpl/configModule ${LOG4CPLUS_INCLUDE_DIR} )


set( SulDownloaderData  ${CMAKE_SOURCE_DIR}/modules/SulDownloader/suldownloaderdata)

include_directories(SYSTEM BEFORE PUBLIC ${PROTOBUF_INCLUDE_DIR})

set( ENV{LD_LIBRARY_PATH} ${Protobuf_LD_LIBRARY_PATH} )

PROTOBUF_GENERATE_CPP(CONFIG_SETTINGS_PROTO_SRC CONFIG_SETTINGS_PROTO_H ${SulDownloaderData}/proto/ConfigurationSettings.proto)
PROTOBUF_GENERATE_CPP(DOWNLOADER_REPORT_PROTO_SRC DOWNLOADER_REPORT_PROTO_H ${SulDownloaderData}/proto/DownloadReport.proto)

add_executable(suldownloaderconfigtests
        suldownloaderconfigtests.cpp
        ${SulDownloaderData}/ConfigurationData.h
        ${SulDownloaderData}/ConfigurationData.cpp
        ${SulDownloaderData}/Proxy.h
        ${SulDownloaderData}/Proxy.cpp
        ${SulDownloaderData}/Credentials.h
        ${SulDownloaderData}/Credentials.cpp
        ${SulDownloaderData}/SulDownloaderException.h
        ${SulDownloaderData}/SulDownloaderException.cpp
        ${SulDownloaderData}/Logger.h
        ${SulDownloaderData}/Logger.cpp
        ${CONFIG_SETTINGS_PROTO_SRC}
        ${CONFIG_SETTINGS_PROTO_H}
        $<TARGET_OBJECTS:loggingobject>
        $<TARGET_OBJECTS:filesystemimplobject>
        $<TARGET_OBJECTS:applicationconfigurationimplobject>

        )
target_link_libraries(suldownloaderconfigtests PRIVATE
        ${LOG4CPLUS_LIBRARY}
        ${Protobuf_LIBRARIES}
        utilityimpl
        obfuscationimpl
        )

SET_TARGET_PROPERTIES( suldownloaderconfigtests
        PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

target_include_directories(suldownloaderconfigtests PUBLIC ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/modules/SulDownloader/suldownloaderdata ${LOG4CPLUS_INCLUDE_DIR}   ${CMAKE_CURRENT_BINARY_DIR})


add_executable(suldownloaderreporttests
        suldownloaderreporttests.cpp
        )

target_link_libraries(suldownloaderreporttests PRIVATE
        suldownloaderdata
        )

SET_TARGET_PROPERTIES( suldownloaderreporttests
        PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

target_include_directories(suldownloaderreporttests PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${CMAKE_SOURCE_DIR}/modules/Common
        ${CMAKE_SOURCE_DIR}/modules/SulDownloader/suldownloaderdata
        ${LOG4CPLUS_INCLUDE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR})




