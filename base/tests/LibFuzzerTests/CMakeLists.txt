
set(PROTO_INPUT_FILE  message.proto )

protobuf_generate_cpp(LIB_FUZZER_EXAMPLE_PROTO_SRCS
        LIB_FUZZER_EXAMPLE_PROTO_HDRS
        ${PROTO_INPUT_FILE} ${CMAKE_SOURCE_DIR}/modules/Common/PluginProtocol/proto/PluginAPIMessage.proto)
SET( MutatorPath ${CMAKE_SOURCE_DIR}/thirdparty/output )
SET( MutatorInclude ${MutatorPath}/include )
SET( MutatorLibs ${MutatorPath}/lib/libprotobuf-mutator-libfuzzer.a ${MutatorPath}/lib/libprotobuf-mutator.a  )


add_executable(ManagementAgentApiTest
        ManagementAgentApiTest.cpp
)

# The different path for BUILD_FUZZ_TESTS and not is to enable developers running on machines without clang
# to be able to use the IDE. It will build a ManagementAgentApiTest without the fuzzer but capable of running
# a single input.
if (BUILD_FUZZ_TESTS )
    target_compile_options(ManagementAgentApiTest PRIVATE -DHasLibFuzzer -Wno-unused-value -Wno-unused-parameter)
    target_link_libraries(ManagementAgentApiTest ${MutatorLibs})
    target_include_directories(ManagementAgentApiTest PRIVATE ${MutatorInclude} ${MutatorInclude}/libprotobuf-mutator)
else()
    target_compile_options(ManagementAgentApiTest PRIVATE  -Wno-unused-value -Wno-unused-parameter)
endif()


add_library(fuzzer-proto ${LIB_FUZZER_EXAMPLE_PROTO_SRCS})


set_property(TARGET fuzzer-proto
        PROPERTY COMPILE_FLAGS ${NO_FUZZING_FLAGS})

target_include_directories(fuzzer-proto PUBLIC ${Protobuf_INCLUDE_DIR})

target_include_directories(ManagementAgentApiTest PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR})
target_link_libraries(ManagementAgentApiTest fuzzer-proto  managementagentimpl
        ${Protobuf_LIBRARIES}
        )

