sophos_add_library(eventjournal SHARED
	EventJournalWriter.cpp
	EventJournalWriter.h
	IEventJournalWriter.h
	Logger.h
	Logger.cpp
	EXTRA_INCLUDES ${pluginapiinclude} ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/modules ${JOURNAL_INCLUDE_DIR} ${CAPNPROTO_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR}
	EXTRA_LIBS ${JOURNAL_LIBRARY} ${LOG4CPLUS_LIBRARY}
		"${CAPNPROTO_LIBRARY_DIR}/libcapnpc.a"
		"${CAPNPROTO_LIBRARY_DIR}/libcapnp-rpc.a"
		"${CAPNPROTO_LIBRARY_DIR}/libcapnp.a"
		"${CAPNPROTO_LIBRARY_DIR}/libkj-async.a"
		"${CAPNPROTO_LIBRARY_DIR}/libkj.a")
