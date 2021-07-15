add_library(eventjournal
	EventJournalWriter.cpp
	EventJournalWriter.h
	IEventJournalWriter.h
	EventJournalTimeUtils.h
	Logger.h
	Logger.cpp)

target_include_directories(eventjournal PUBLIC ${pluginapiinclude} ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/modules ${JOURNAL_INCLUDE_DIR} ${CAPNPROTO_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(eventjournal PUBLIC ${JOURNAL_LIBRARY} ${LOG4CPLUS_LIBRARY}
	"${CAPNPROTO_LIBRARY_DIR}/libcapnpc.a"
	"${CAPNPROTO_LIBRARY_DIR}/libcapnp-rpc.a"
	"${CAPNPROTO_LIBRARY_DIR}/libcapnp.a"
	"${CAPNPROTO_LIBRARY_DIR}/libkj-async.a"
	"${CAPNPROTO_LIBRARY_DIR}/libkj.a")
