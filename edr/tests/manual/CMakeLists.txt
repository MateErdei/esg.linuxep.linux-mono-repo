set( GOOGLETESTINCLUDE ${CMAKE_SOURCE_DIR}/tests/googletest/googletest/include )

add_executable(LiveQueryReport
        LiveQueryReport.cpp
        )
target_link_libraries(LiveQueryReport PUBLIC osqueryclient
        ${GLOG_LIBRARY}
        ${GFLAG_LIBRARY}
        ${OSQUERY_LIBRARY}
        ${THRIFT_LIBRARY}
        ${BOOST_CHRONO_LIBRARY}
        ${BOOST_SYSTEM_LIBRARY}
        ${BOOST_THREAD_LIBRARY}
        ${BOOST_FILESYSTEM_LIBRARY}
        ${BOOST_CONTEXT_LIBRARY}
        ${OPENSSLCRYPTO_LIBRARY}
        ${OPENSSL_LIBRARY}
        ${zmqlib}
        ${log4cpluslib}
        ${protobuflib}
        )
target_include_directories(LiveQueryReport PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/modules
        ${OSQUERYSDK}/include
        ${GOOGLETESTINCLUDE}
        ${BOOST_INCLUDE_DIR}
        )

SET_TARGET_PROPERTIES( LiveQueryReport
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS LiveQueryReport
        DESTINATION ../componenttests)


add_executable(DelayControlledTable
        DelayControlledTable.cpp
        )
target_link_libraries(DelayControlledTable PUBLIC
        ${GLOG_LIBRARY}
        ${GFLAG_LIBRARY}
        ${OSQUERY_LIBRARY}
        ${THRIFT_LIBRARY}
        ${BOOST_CHRONO_LIBRARY}
        ${BOOST_SYSTEM_LIBRARY}
        ${BOOST_THREAD_LIBRARY}
        ${BOOST_FILESYSTEM_LIBRARY}
        ${BOOST_CONTEXT_LIBRARY}
        ${OPENSSLCRYPTO_LIBRARY}
        ${OPENSSL_LIBRARY}
        )

target_include_directories(DelayControlledTable PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${OSQUERYSDK}/include
        ${GOOGLETESTINCLUDE}
        ${BOOST_INCLUDE_DIR}
        )

SET_TARGET_PROPERTIES( DelayControlledTable
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS DelayControlledTable
        DESTINATION ../componenttests)


add_executable(BinaryDataTable
        BinaryDataTable.cpp
        )
target_link_libraries(BinaryDataTable PUBLIC
        ${GLOG_LIBRARY}
        ${GFLAG_LIBRARY}
        ${OSQUERY_LIBRARY}
        ${THRIFT_LIBRARY}
        ${BOOST_CHRONO_LIBRARY}
        ${BOOST_SYSTEM_LIBRARY}
        ${BOOST_THREAD_LIBRARY}
        ${BOOST_FILESYSTEM_LIBRARY}
        ${BOOST_CONTEXT_LIBRARY}
        ${OPENSSLCRYPTO_LIBRARY}
        ${OPENSSL_LIBRARY}
        )

target_include_directories(BinaryDataTable PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${OSQUERYSDK}/include
        ${GOOGLETESTINCLUDE}
        ${BOOST_INCLUDE_DIR}
        )

SET_TARGET_PROPERTIES(BinaryDataTable
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS BinaryDataTable
        DESTINATION ../componenttests)



