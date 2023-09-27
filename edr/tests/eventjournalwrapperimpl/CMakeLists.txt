SophosAddTest(TestEventJournalWrapperImpl
        MockSophosJournal.h
        TestEventJournalReaderWrapper.cpp
        PROJECTS eventjournalwrapperimpl
        LIBS ${log4cpluslib} ${pluginapilib} ${testhelperslib} pthread
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${testhelpersinclude}
        )

add_dependencies(TestEventJournalWrapperImpl copy_libs)

SET_TARGET_PROPERTIES(TestEventJournalWrapperImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

target_compile_definitions(TestEventJournalWrapperImpl PUBLIC -D_HAS_STD_BYTE)
target_include_directories(TestEventJournalWrapperImpl SYSTEM
        PUBLIC ${JOURNAL_INCLUDE_DIR} ${CAPNPROTO_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(TestEventJournalWrapperImpl PUBLIC ${JOURNAL_LIBRARY}  ${log4cpluslib} ${LZMA_LIBRARY} stdc++fs
        "${CAPNPROTO_LIBRARY_DIR}/libcapnpc.a"
        "${CAPNPROTO_LIBRARY_DIR}/libcapnp-rpc.a"
        "${CAPNPROTO_LIBRARY_DIR}/libcapnp.a"
        "${CAPNPROTO_LIBRARY_DIR}/libkj-async.a"
        "${CAPNPROTO_LIBRARY_DIR}/libkj.a")
