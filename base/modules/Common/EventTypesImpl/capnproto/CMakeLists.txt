INCLUDE_DIRECTORIES(${CAPNPROTO_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set( LD ${CAPNPROTO_LIBRARY_DIR}:${LD_LIBRARY_PATH} )
set( CAPNPATH ${CAPNPROTO_EXECUTABLE_DIR}:${PATH} )
set( COMMANDSTRING "LD_LIBRARY_PATH=${LD} PATH=${CAPNPATH} ${CAPNPROTO_EXECUTABLE} compile -oc++ --src-prefix=${CMAKE_CURRENT_LIST_DIR}")


add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Common.capnp.h ${CMAKE_CURRENT_BINARY_DIR}/Common.capnp.c++ ${CMAKE_CURRENT_BINARY_DIR}/PortScanning.capnp.c++ ${CMAKE_CURRENT_BINARY_DIR}/PortScanning.capnp.h ${CMAKE_CURRENT_BINARY_DIR}/Credentials.capnp.c++ ${CMAKE_CURRENT_BINARY_DIR}/Credentials.capnp.h
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CAPNPROTO_INCLUDE_DIR}/capnp capnp
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR} .
        COMMAND /bin/bash -c "${COMMANDSTRING} *.capnp"
)
#set( CapnpGeneratedFiles Credentials.capnp.h Credentials.capnp.c++)

#
#add_custom_target(capnproto_compile
#        #PRE_BUILD
#        #OUTPUT capnproto_compile
#        COMMENT "COMPILING CAPNPROTO Definitiosn"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Common.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Credentials.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Directory.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Dns.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Driver.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Event.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/File.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/FileHash.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/FileProperty.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Http.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Image.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Ip.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Network.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Network.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/PortScanning.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Process.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/ProcessProperty.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Registry.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/System.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Thread.capnp"
#        COMMAND bash -c "${COMMANDSTRING} ${CMAKE_CURRENT_LIST_DIR}/Url.capnp"
#        configure_file( ${CMAKE_CURRENT_BINARY_DIR}/Credentials.capnp.h ${CMAKE_CURRENT_LIST_DIR} COPYONLY)
#        configure_file( ${CMAKE_CURRENT_BINARY_DIR}/Credentials.capnp.c++ ${CMAKE_CURRENT_LIST_DIR} COPYONLY)
#        configure_file( ${CMAKE_CURRENT_BINARY_DIR}/PortScanning.capnp.h ${CMAKE_CURRENT_LIST_DIR} COPYONLY)
#        configure_file( ${CMAKE_CURRENT_BINARY_DIR}/PortScanning.capnp.c++ ${CMAKE_CURRENT_LIST_DIR} COPYONLY)
#        configure_file( ${CMAKE_CURRENT_BINARY_DIR}/Common.capnp.c++ ${CMAKE_CURRENT_LIST_DIR} COPYONLY)
#        configure_file( ${CMAKE_CURRENT_BINARY_DIR}/Common.capnp.h ${CMAKE_CURRENT_LIST_DIR} COPYONLY)
#        #set( PortScanInclude ${CMAKE_CURRENT_LIST_DIR}/PortScanning.capnp.h )
#)

#add_custom_target(
#        capnproto_copy_output #capnproto_compile
#        configure_file( ${CMAKE_CURRENT_BINARY_DIR}/Credentials.capnp.h ${CMAKE_CURRENT_LIST_DIR} COPYONLY)
#        configure_file( ${CMAKE_CURRENT_BINARY_DIR}/Credentials.capnp.c++ ${CMAKE_CURRENT_LIST_DIR} COPYONLY)
#        configure_file( ${CMAKE_CURRENT_BINARY_DIR}/PortScanning.capnp.h ${CMAKE_CURRENT_LIST_DIR} COPYONLY)
#        configure_file( ${CMAKE_CURRENT_BINARY_DIR}/PortScanning.capnp.c++ ${CMAKE_CURRENT_LIST_DIR} COPYONLY)
#        configure_file( ${CMAKE_CURRENT_BINARY_DIR}/Common.capnp.c++ ${CMAKE_CURRENT_LIST_DIR} COPYONLY)
#        configure_file( ${CMAKE_CURRENT_BINARY_DIR}/Common.capnp.h ${CMAKE_CURRENT_LIST_DIR} COPYONLY)
#        #DEPENDS capnproto_compile
#)





