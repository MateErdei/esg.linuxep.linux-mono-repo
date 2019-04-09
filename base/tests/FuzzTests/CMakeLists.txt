
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