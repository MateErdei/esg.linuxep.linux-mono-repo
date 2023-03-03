add_executable(FuzzableResultSender
        FuzzableResultSender.cpp
        ${SQLITE_INCLUDE_DIR}/sqlite3.c

        )

target_link_libraries(FuzzableResultSender PUBLIC
        OsqueryExtensions
        eventjournalwrapperimpl
        ${pluginapilib}
        ${protobuflib}
        ${log4cpluslib}
        ${GLOG_LIBRARY}
        ${BOOST_CHRONO_LIBRARY}
        ${BOOST_SYSTEM_LIBRARY}
        ${BOOST_THREAD_LIBRARY}
        ${BOOST_FILESYSTEM_LIBRARY}
        ${BOOST_CONTEXT_LIBRARY}
        ${OPENSSLCRYPTO_LIBRARY}
        ${OPENSSL_LIBRARY}
        ${JSONCPP_LIBRARY}
        pthread
        ${CMAKE_DL_LIBS}
        dl
        ${GFLAG_LIBRARY}
        eventjournalwrapperimpl
        ${LZMA_LIBRARY}
        stdc++fs
        )
target_compile_definitions(FuzzableResultSender PUBLIC -D_HAS_STD_BYTE)
target_include_directories(FuzzableResultSender PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${GOOGLETESTINCLUDE}
        ${BOOST_INCLUDE_DIR}
        ${SQLITE_INCLUDE_DIR}
        ${INPUT}/jsoncpp/include
        ${pluginapiinclude}
        ${CMAKE_SOURCE_DIR}
        ${JOURNAL_INCLUDE_DIR}
        ${CAPNPROTO_INCLUDE_DIR}
        ${LZMA_INCLUDE_DIR}
        )

SET_TARGET_PROPERTIES(FuzzableResultSender
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS FuzzableResultSender
        DESTINATION ../componenttests)
