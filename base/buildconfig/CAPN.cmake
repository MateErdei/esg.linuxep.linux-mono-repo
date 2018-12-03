
function(CAPN_GENERATE_CPP SRCS HDRS)
  cmake_parse_arguments(capnp "" "" "SEPARATION" ${ARGN})
  set( LD ${CAPNPROTO_LIBRARY_DIR}:${LD_LIBRARY_PATH} )
  set( CAPNPATH ${CAPNPROTO_EXECUTABLE_DIR}:${PATH} )

  set(${SRCS})
  set(${HDRS})

  set(PROTO_FILES "${capnp_UNPARSED_ARGUMENTS}")
    message("The proto files: ${PROTO_FILES}" )
  if(NOT PROTO_FILES)
    message(SEND_ERROR "Error: CAPN_GENERATE_CPP() called without any proto files")
    return()
  endif()

  add_custom_command(
          OUTPUT capnp
          "${_protobuf_protoc_hdr}"
          COMMAND ${CMAKE_COMMAND} -E copy_directory ${CAPNPROTO_INCLUDE_DIR}/capnp capnp
          COMMENT "Copying capnp to build directory"
          VERBATIM )

  foreach(FIL ${PROTO_FILES})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)

    set(_protobuf_protoc_src "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.capnp.c++")
    set(_protobuf_protoc_hdr "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.capnp.h")
    list(APPEND ${SRCS} "${_protobuf_protoc_src}")
    list(APPEND ${HDRS} "${_protobuf_protoc_hdr}")
    add_custom_command(
            OUTPUT "${_protobuf_protoc_src}"
            "${_protobuf_protoc_hdr}"
            COMMAND ${CMAKE_COMMAND} -E copy ${ABS_FIL} .
            COMMAND ldd ${CAPNPROTO_EXECUTABLE}
            COMMAND  ${CMAKE_COMMAND} "${CAPNPROTO_EXECUTABLE}"
            compile "-oc++"
             ${FIL_WE}.capnp
            DEPENDS ${ABS_FIL} capnp
            COMMENT "Running C++ capn compile buffer compiler on ${FIL} with LIBRARY_PATH $ENV{LD_LIBRARY_PATH}"
            VERBATIM )
  endforeach()

  set(${SRCS} "${${SRCS}}" PARENT_SCOPE)
  set(${HDRS} "${${HDRS}}" PARENT_SCOPE)
endfunction()
