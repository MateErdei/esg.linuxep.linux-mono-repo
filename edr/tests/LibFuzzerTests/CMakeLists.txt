
set(PROTO_INPUT_FILE livequery.proto)

protobuf_generate_cpp(LIB_FUZZER_EXAMPLE_PROTO_SRCS
        LIB_FUZZER_EXAMPLE_PROTO_HDRS
        ${PROTO_INPUT_FILE})

SET( MutatorPath ${CMAKE_SOURCE_DIR}/thirdparty/output )
SET( MutatorInclude ${MutatorPath}/include )
SET( MutatorLibs ${MutatorPath}/lib/libprotobuf-mutator-libfuzzer.a ${MutatorPath}/lib/libprotobuf-mutator.a  )


add_library(fuzzer-proto
        FuzzerUtils.h
        FuzzerUtils.cpp
        ${LIB_FUZZER_EXAMPLE_PROTO_SRCS}
        )


set_property(TARGET fuzzer-proto
        PROPERTY COMPILE_FLAGS ${NO_FUZZING_FLAGS}
        )

target_include_directories(fuzzer-proto PUBLIC
        ${Protobuf_INCLUDE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}
        )
target_link_libraries(fuzzer-proto PUBLIC ${Protobuf_LIBRARIES})


# The different path for BUILD_FUZZ_TESTS and not is to enable developers running on machines without clang
# to be able to use the IDE. It will build a ManagementAgentApiTest without the fuzzer but capable of running
# a single input.
if (BUILD_FUZZ_TESTS )
    target_compile_options(fuzzer-proto PUBLIC -DHasLibFuzzer -Wno-unused-value -Wno-unused-parameter)
    target_link_libraries(fuzzer-proto PUBLIC ${MutatorLibs})
    target_include_directories(fuzzer-proto PUBLIC ${MutatorInclude} ${MutatorInclude}/libprotobuf-mutator  ${pluginapiinclude})
else()
    target_compile_options(fuzzer-proto PUBLIC  -Wno-unused-value -Wno-unused-parameter)
    target_include_directories(fuzzer-proto PUBLIC ${pluginapiinclude})
endif()


sophos_add_exe(LiveQueryTests
        LiveQueryTests.cpp
        EXTRA_PROJECTS fuzzer-proto osqueryclient livequeryimpl
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude}
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib} ${expatlib} ${zlib}
        )
