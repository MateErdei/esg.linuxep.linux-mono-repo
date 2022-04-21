
add_library(centralregistrationapi SHARED
        $<TARGET_OBJECTS:centralregistrationimpl>
        $<TARGET_OBJECTS:mcsimpl>
        )

#target_link_libraries(pluginapi PUBLIC zmq log4cplus protobuf expat -L${EXPAT_INPUT}/lib64 -L${ZEROMQ_INPUT}/lib -L${Protobuf_root}/lib -L${LOG4CPLUS_INPUT}/lib)

#add_library(sharedtesthelpers SHARED $<TARGET_OBJECTS:testhelpersobject>)
#target_link_libraries(sharedtesthelpers PUBLIC pluginapi)

#SET_TARGET_PROPERTIES( sharedtesthelpers
#        PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../../lib64")

SET_TARGET_PROPERTIES( centralregistrationapi
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN")

#INCLUDE(FindLibraryFullPath)
#FindLibraryFullPath( zmqfull zmq ${ZEROMQ_INPUT}/lib)
#FindLibraryFullPath( log4cplusfull log4cplus ${LOG4CPLUS_INPUT}/lib)
#FindLibraryFullPath( protobuffull protobuf ${Protobuf_root}/lib)
#FindLibraryFullPath( expatfull   expat  ${EXPAT_INPUT}/lib64)
#FindLibraryFullPath( zlibfull  z   ${ZLIB_INPUT}/lib64)
#configure_file(PluginAPICommitHash.in PluginAPICommitHash @ONLY)

add_custom_target(
        centralregistrationapitar ALL
        COMMAND mkdir -p centralregistrationapi/include/CentralRegistration
        COMMAND mkdir -p centralregistrationapi/include/cmcsrouter
        COMMAND mkdir -p centralregistrationapi/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/modules/CentralRegistration/libcentralregistrationimpl.so centralregistrationapi/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/modules/cmcsrouter/libmcsimpl.so centralregistrationapi/lib64
)

function(add_copy_directory base)
    file(GLOB_RECURSE tempfiles
            LIST_DIRECTORIES false
            ../../modules/${base}/*.h
            )
    add_custom_command(
            TARGET centralregistrationapitar
            POST_BUILD
            #            COMMAND echo ${base}
            #            COMMAND echo ${tempfiles}
            COMMAND ${CMAKE_COMMAND} -E copy ${tempfiles} centralregistrationapi/include/${base}/
    )
endfunction()

function(add_copy_test_directory base)
    file(GLOB_RECURSE tempfiles
            LIST_DIRECTORIES false
            ../../tests/${base}/*.h
            )
    add_custom_command(
            TARGET centralregistrationapitar
            POST_BUILD
            #            COMMAND echo ${base}
            #            COMMAND echo ${tempfiles}
            COMMAND ${CMAKE_COMMAND} -E copy ${tempfiles} centralregistrationapi/tests/include/${base}/
    )
endfunction()

add_copy_directory(cmcsrouter)
add_copy_directory(CentralRegistration)


#add_custom_command(
#        TARGET centralregistrationapitar
#        POST_BUILD
#        COMMAND mkdir -p centralregistrationapi/lib64
#        COMMAND ${CMAKE_COMMAND} -E copy ${distributionfiles} centralregistrationapi/lib64/
#)

add_custom_command(
        TARGET centralregistrationapitar
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar czf centralregistrationapi.tar.gz centralregistrationapi
        COMMAND rm -rf centralregistrationapi
)

