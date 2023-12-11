
add_library(cmcsrouterapi SHARED
        $<TARGET_OBJECTS:HttpRequestsImplobject>
        $<TARGET_OBJECTS:applicationconfigurationimplobject>
        $<TARGET_OBJECTS:centralregistrationimpl>
        $<TARGET_OBJECTS:filesystemimplobject>
        $<TARGET_OBJECTS:loggingobject>
        $<TARGET_OBJECTS:mcsimpl>
        $<TARGET_OBJECTS:obfuscationimplobject>
        $<TARGET_OBJECTS:osutilitiesimpl>
        $<TARGET_OBJECTS:policyobject>
        $<TARGET_OBJECTS:protobufutilobject>
        $<TARGET_OBJECTS:threadsobject>
        $<TARGET_OBJECTS:utilityimplobject>
        $<TARGET_OBJECTS:xmlutilitiesobject>
        )

target_link_libraries(cmcsrouterapi PUBLIC
        zmq -L${ZEROMQ_INPUT}
        log4cplus -L${LOG4CPLUS_INPUT}
        protobuf -L${Protobuf_root}/lib
        expat -L${EXPAT_INPUT}
        sslimplobject
        cap
        sophlib_hostname
        )


SET_TARGET_PROPERTIES( cmcsrouterapi
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN")

INCLUDE(FindLibraryFullPath)
FindLibraryFullPath( zmqfull zmq ${ZEROMQ_INPUT})
FindLibraryFullPath( log4cplusfull log4cplus ${LOG4CPLUS_INPUT})
FindLibraryFullPath( protobuffull protobuf ${Protobuf_root}/lib)
FindLibraryFullPath( expatfull expat ${EXPAT_INPUT})

add_custom_target(
        cmcsrouterapitar ALL
        COMMAND mkdir -p mcslibrary/include mcslibrary/lib64
        COMMAND cp ${NLOHMANN_JSON_INCLUDE_DIR}/nlohmann/json.hpp mcslibrary/include
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cmcsrouterapi> mcslibrary/lib64
        COMMAND chmod +x ${CMAKE_SOURCE_DIR}/products/Base/installer/rpath_lib.sh
        COMMAND ${CMAKE_SOURCE_DIR}/products/Base/installer/rpath_lib.sh  mcslibrary/lib64/libcmcsrouterapi.so
        COMMAND ${CMAKE_COMMAND} -E copy ${zmqfull} mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${log4cplusfull} mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${protobuffull} mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy ${expatfull}    mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:bzip2> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:lzma> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:minizip> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:zlib> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:openssl_ssl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:openssl_crypto> mcslibrary/lib64
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
add_copy_directory(Common/Policy)
add_copy_directory(Common/ObfuscationImpl)

