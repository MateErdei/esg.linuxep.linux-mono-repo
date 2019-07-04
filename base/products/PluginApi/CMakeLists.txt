
add_library(pluginapi SHARED
        $<TARGET_OBJECTS:pluginapiobjects>
        $<TARGET_OBJECTS:pluginprotocolobject>
        $<TARGET_OBJECTS:zeromqwrapperimplobject>
        $<TARGET_OBJECTS:zmqwrapperapiimplobject>
        $<TARGET_OBJECTS:reactorimplobject>
        $<TARGET_OBJECTS:threadsobject>
        $<TARGET_OBJECTS:filesystemimplobject>
        $<TARGET_OBJECTS:applicationconfigurationimplobject>
        $<TARGET_OBJECTS:loggingobject>
        $<TARGET_OBJECTS:eventtypesimplobject>
        $<TARGET_OBJECTS:telemetryhelperimplobject>
        $<TARGET_OBJECTS:processmonitoringimplobject>
        $<TARGET_OBJECTS:processimplobject>
        $<TARGET_OBJECTS:utilityimplobject>
        $<TARGET_OBJECTS:xmlutilitiesobject>
        )

target_link_libraries(pluginapi PUBLIC zmq log4cplus protobuf -L${ZEROMQ_INPUT}/lib -L${Protobuf_root}/lib -L${LOG4CPLUS_INPUT}/lib
        "${CAPNPROTO_LIBRARY_DIR}/libcapnpc.a"
        "${CAPNPROTO_LIBRARY_DIR}/libcapnp-rpc.a"
        "${CAPNPROTO_LIBRARY_DIR}/libcapnp.a"
        "${CAPNPROTO_LIBRARY_DIR}/libkj-async.a"
        "${CAPNPROTO_LIBRARY_DIR}/libkj.a"
        )

add_library(sharedtesthelpers SHARED $<TARGET_OBJECTS:testhelpersobject>)
target_link_libraries(sharedtesthelpers PUBLIC pluginapi)

SET_TARGET_PROPERTIES( sharedtesthelpers
        PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../../lib64")

SET_TARGET_PROPERTIES( pluginapi
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN")

INCLUDE(FindLibraryFullPath)
FindLibraryFullPath( zmqfull zmq ${ZEROMQ_INPUT}/lib)
FindLibraryFullPath( log4cplusfull log4cplus ${LOG4CPLUS_INPUT}/lib)
FindLibraryFullPath( protobuffull protobuf ${Protobuf_root}/lib)

add_custom_target(
        plugintar ALL
        COMMAND mkdir -p pluginapi/include pluginapi/lib64
        COMMAND mkdir -p pluginapi/include/Common/ApplicationConfiguration
        COMMAND mkdir -p pluginapi/include/Common/FileSystem
        COMMAND mkdir -p pluginapi/include/Common/Logging
        COMMAND mkdir -p pluginapi/include/Common/Process
        COMMAND mkdir -p pluginapi/include/Common/ProcessMonitoring
        COMMAND mkdir -p pluginapi/include/Common/PluginApi
        COMMAND mkdir -p pluginapi/include/Common/Reactor
        COMMAND mkdir -p pluginapi/include/Common/TelemetryHelperImpl
        COMMAND mkdir -p pluginapi/include/Common/Threads
        COMMAND mkdir -p pluginapi/include/Common/Exceptions
        COMMAND mkdir -p pluginapi/include/Common/XmlUtilities
        COMMAND mkdir -p pluginapi/include/Common/ZeroMQWrapper
        COMMAND mkdir -p pluginapi/include/Common/ZMQWrapperApi
        COMMAND mkdir -p pluginapi/tests/include/Common/Helpers
        COMMAND mkdir -p pluginapi/include/Common/EventTypes
        COMMAND mkdir -p pluginapi/tests/lib64
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${LOG4CPLUS_INPUT}/include/log4cplus pluginapi/include/log4cplus
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pluginapi> pluginapi/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${zmqfull} pluginapi/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${log4cplusfull} pluginapi/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${protobuffull} pluginapi/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sharedtesthelpers> pluginapi/tests/lib64
        DEPENDS testhelpers
)

function(add_copy_directory base)
    file(GLOB_RECURSE tempfiles
            LIST_DIRECTORIES false
            ../../modules/${base}/*.h
            )
    add_custom_command(
            TARGET plugintar
            POST_BUILD
            #            COMMAND echo ${base}
            #            COMMAND echo ${tempfiles}
            COMMAND ${CMAKE_COMMAND} -E copy ${tempfiles} pluginapi/include/${base}/
    )
endfunction()

function(add_copy_test_directory base)
    file(GLOB_RECURSE tempfiles
            LIST_DIRECTORIES false
            ../../tests/${base}/*.h
            )
    add_custom_command(
            TARGET plugintar
            POST_BUILD
            #            COMMAND echo ${base}
            #            COMMAND echo ${tempfiles}
            COMMAND ${CMAKE_COMMAND} -E copy ${tempfiles} pluginapi/tests/include/${base}/
    )
endfunction()

add_copy_directory(Common/ApplicationConfiguration)
add_copy_directory(Common/FileSystem)
add_copy_directory(Common/Logging)
add_copy_directory(Common/PluginApi)
add_copy_directory(Common/Process)
add_copy_directory(Common/ProcessMonitoring)
add_copy_directory(Common/Reactor)
add_copy_directory(Common/TelemetryHelperImpl)
add_copy_directory(Common/Threads)
add_copy_directory(Common/ZeroMQWrapper)
add_copy_directory(Common/ZMQWrapperApi)
add_copy_directory(Common/EventTypes)
add_copy_directory(Common/Exceptions)
add_copy_directory(Common/XmlUtilities)
add_copy_test_directory(Common/Helpers)


file(GLOB_RECURSE distributionfiles
        LIST_DIRECTORIES false
        ${CMAKE_SOURCE_DIR}/products/distribution/*.py
        ${CMAKE_SOURCE_DIR}/products/distribution/addSymbols.sh
        )

add_custom_command(
        TARGET plugintar
        POST_BUILD
        COMMAND mkdir -p pluginapi/distribution
        COMMAND ${CMAKE_COMMAND} -E copy ${distributionfiles} pluginapi/distribution/
)

add_custom_command(
        TARGET plugintar
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar czf pluginapi.tar.gz pluginapi
        COMMAND rm -rf pluginapi
)

