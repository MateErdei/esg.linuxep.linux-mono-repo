
add_library(cmcsrouterapi SHARED
        $<TARGET_OBJECTS:HttpRequestsImpl>
        $<TARGET_OBJECTS:applicationconfigurationimpl>
        $<TARGET_OBJECTS:applicationconfigurationimplobject>
        $<TARGET_OBJECTS:centralregistrationimpl>
        $<TARGET_OBJECTS:filesystemimpl>
        $<TARGET_OBJECTS:filesystemimplobject>
        $<TARGET_OBJECTS:httpsenderimpl>
        $<TARGET_OBJECTS:logging>
        $<TARGET_OBJECTS:loggingobject>
        $<TARGET_OBJECTS:mcsimpl>
        $<TARGET_OBJECTS:obfuscationimpl>
        $<TARGET_OBJECTS:osutilitiesimpl>
        $<TARGET_OBJECTS:sslimpl>
        $<TARGET_OBJECTS:threadsobject>
        $<TARGET_OBJECTS:utilityimpl>
        $<TARGET_OBJECTS:utilityimplobject>
        $<TARGET_OBJECTS:xmlutilities>
        $<TARGET_OBJECTS:xmlutilitiesobject>
        )

target_link_libraries(cmcsrouterapi PUBLIC zmq log4cplus protobuf expat -L${EXPAT_INPUT}/lib64 -L${ZEROMQ_INPUT}/lib -L${Protobuf_root}/lib -L${LOG4CPLUS_INPUT}/lib)


SET_TARGET_PROPERTIES( cmcsrouterapi
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN")

INCLUDE(FindLibraryFullPath)
FindLibraryFullPath( zmqfull zmq ${ZEROMQ_INPUT}/lib)
FindLibraryFullPath( log4cplusfull log4cplus ${LOG4CPLUS_INPUT}/lib)
FindLibraryFullPath( protobuffull protobuf ${Protobuf_root}/lib)
FindLibraryFullPath( expatfull   expat  ${EXPAT_INPUT}/lib64)
FindLibraryFullPath( zlibfull  z   ${ZLIB_INPUT}/lib64)

add_custom_target(
        cmcsrouterapitar ALL
        COMMAND mkdir -p mcslibrary/include mcslibrary/lib64
        COMMAND cp ${NLOHMANN_JSON_INCLUDE_DIR}/json.hpp mcslibrary/include
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cmcsrouterapi> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${zmqfull} mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${log4cplusfull} mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${protobuffull} mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${expatfull}    mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${zlibfull}    mcslibrary/lib64
)

function(add_copy_directory base)
    file(GLOB_RECURSE tempfiles
            LIST_DIRECTORIES false
            ../../modules/${base}/*.h
            )
    add_custom_command(
            TARGET cmcsrouterapitar
            POST_BUILD
            #            COMMAND echo ${base}
            #            COMMAND echo ${tempfiles}
            COMMAND mkdir -p mcslibrary/include/${base}
            COMMAND ${CMAKE_COMMAND} -E copy ${tempfiles} mcslibrary/include/${base}/
    )
endfunction()

add_copy_directory(cmcsrouter)
add_copy_directory(CentralRegistration)
add_copy_directory(Common/OSUtilities)
add_copy_directory(Common/CurlWrapper)
add_copy_directory(Common/HttpRequests)
add_copy_directory(Common/HttpRequestsImpl)
add_copy_directory(Common/HttpSenderImpl)
