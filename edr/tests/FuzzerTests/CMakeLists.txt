
set(PROTO_INPUT_FILE livequery.proto livequeryinput.proto)

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


if (BUILD_FOR_DEVELOPER_DEBUG )
    target_compile_options(fuzzer-proto PUBLIC  -Wno-unused-value -Wno-unused-parameter)
    target_include_directories(fuzzer-proto PUBLIC ${pluginapiinclude})
else()
    target_compile_options(fuzzer-proto PUBLIC -DHasLibFuzzer -Wno-unused-value -Wno-unused-parameter)
    target_link_libraries(fuzzer-proto PUBLIC ${MutatorLibs})
    target_include_directories(fuzzer-proto PUBLIC ${MutatorInclude} ${MutatorInclude}/libprotobuf-mutator  ${pluginapiinclude})
endif()


sophos_add_exe(LiveQueryTests
        LiveQueryTests.cpp
        EXTRA_PROJECTS fuzzer-proto osqueryclient livequeryimpl pluginimpl
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude}  ${testhelpersinclude}
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib} ${expatlib} ${zlib}  ${testhelperslib}
        )
sophos_add_exe(LiveQueryInputTests
	LiveQueryInputTests.cpp
        EXTRA_PROJECTS fuzzer-proto osqueryclient livequeryimpl pluginimpl
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude}  ${testhelpersinclude}
        EXTRA_LIBS ${zmqlib} ${log4cpluslib} ${protobuflib} ${expatlib} ${zlib}  ${testhelperslib}
	)
