sophos_add_library(eventjournalwrapperimpl SHARED
        EventJournalReaderWrapper.cpp
        EventJournalReaderWrapper.h
        IEventJournalReaderWrapper.h
        Logger.cpp
        Logger.h)

target_compile_definitions(eventjournalwrapperimpl PUBLIC -D_HAS_STD_BYTE)
target_include_directories(eventjournalwrapperimpl SYSTEM PUBLIC ${JOURNAL_INCLUDE_DIR} ${CAPNPROTO_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(eventjournalwrapperimpl PUBLIC ${JOURNAL_LIBRARY}  ${log4cpluslib} ${LZMA_LIBRARY} stdc++fs
  "${CAPNPROTO_LIBRARY_DIR}/libcapnpc.a"
  "${CAPNPROTO_LIBRARY_DIR}/libcapnp-rpc.a"
  "${CAPNPROTO_LIBRARY_DIR}/libcapnp.a"
  "${CAPNPROTO_LIBRARY_DIR}/libkj-async.a"
  "${CAPNPROTO_LIBRARY_DIR}/libkj.a")
