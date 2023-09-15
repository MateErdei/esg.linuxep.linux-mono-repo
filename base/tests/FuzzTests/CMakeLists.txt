
add_executable(loggerconfigtests
        loggerconfigtests.cpp
        $<TARGET_OBJECTS:loggingobject>
        $<TARGET_OBJECTS:filesystemimplobject>
        $<TARGET_OBJECTS:applicationconfigurationimplobject>
        $<TARGET_OBJECTS:utilityimplobject>)

target_include_directories(loggerconfigtests PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})

target_link_libraries(loggerconfigtests PUBLIC
        ${LOG4CPLUS_LIBRARY}
        sslimplobject
        cap
        )

SET_TARGET_PROPERTIES( loggerconfigtests
        PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

set(ALCParseDir ${CMAKE_SOURCE_DIR}/modules/UpdateSchedulerImpl/configModule)
add_executable(parsealcpolicytests
        parsealcpolicytests.cpp
        ${ALCParseDir}/UpdatePolicyTranslator.cpp
        ${ALCParseDir}/UpdatePolicyTranslator.h
        ${ALCParseDir}/../common/Logger.cpp
        ${ALCParseDir}/../common/Logger.h
        $<TARGET_OBJECTS:utilityimplobject>
        )
target_link_libraries(parsealcpolicytests PRIVATE
        ${LOG4CPLUS_LIBRARY}
        ${Protobuf_LIBRARIES}
        xmlutilities
        utilityimpl
        osutilitiesimpl
        telemetryhelperimpl
        suldownloaderdata
        policy
        downloadreport
        )

SET_TARGET_PROPERTIES( parsealcpolicytests
        PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

target_include_directories(parsealcpolicytests PUBLIC ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/modules/UpdateSchedulerImpl/configModule ${LOG4CPLUS_INCLUDE_DIR} )

set(SulDownloaderData  ${CMAKE_SOURCE_DIR}/modules/SulDownloader/suldownloaderdata)

include_directories(SYSTEM BEFORE PUBLIC ${PROTOBUF_INCLUDE_DIR})

set(ENV{LD_LIBRARY_PATH} ${Protobuf_LD_LIBRARY_PATH})

PROTOBUF_GENERATE_CPP(DOWNLOADER_REPORT_PROTO_SRC DOWNLOADER_REPORT_PROTO_H ${SulDownloaderData}/proto/DownloadReport.proto)

add_executable(suldownloaderconfigtests
        suldownloaderconfigtests.cpp
        ${SulDownloaderData}/ConfigurationData.h
        ${SulDownloaderData}/ConfigurationData.cpp
        ${SulDownloaderData}/SulDownloaderException.h
        ${SulDownloaderData}/Logger.h
        ${SulDownloaderData}/Logger.cpp
        ${CONFIG_SETTINGS_PROTO_SRC}
        ${CONFIG_SETTINGS_PROTO_H}
        $<TARGET_OBJECTS:loggingobject>
        $<TARGET_OBJECTS:filesystemimplobject>
        $<TARGET_OBJECTS:applicationconfigurationimplobject>
        $<TARGET_OBJECTS:protobufutilobject>
        $<TARGET_OBJECTS:utilityimplobject>
        )

target_link_libraries(suldownloaderconfigtests PRIVATE
        ${LOG4CPLUS_LIBRARY}
        ${Protobuf_LIBRARIES}
        obfuscationimpl
        HttpRequestsImpl
        ProxyUtilsLib
        policy
        )

SET_TARGET_PROPERTIES( suldownloaderconfigtests
        PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

target_include_directories(suldownloaderconfigtests PUBLIC ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/modules/SulDownloader/suldownloaderdata ${LOG4CPLUS_INCLUDE_DIR}   ${CMAKE_CURRENT_BINARY_DIR})


add_executable(downloadreporttests
        downloadreporttests.cpp
        )

target_link_libraries(downloadreporttests PRIVATE
        ${Protobuf_LIBRARIES}
        logging
        policy
        downloadreport
        )

SET_TARGET_PROPERTIES( downloadreporttests
        PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:${CMAKE_BINARY_DIR}/libs"
        )

target_include_directories(downloadreporttests PUBLIC
        ${LOG4CPLUS_INCLUDE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR})




