cmake_minimum_required(VERSION 3.15)
project(untitled)

set(CMAKE_CXX_STANDARD 14)

set(GOOGLETESTINCLUDE ${CMAKE_SOURCE_DIR}/tests/googletest/googletest/include)

add_executable(TestLoggerExtention
        TestLoggerExtention.cpp
        Logger.h
        Logger.cpp
        ../../../modules/pluginimpl/ApplicationPaths.cpp
        ../../../modules/pluginimpl/ApplicationPaths.h
        )

target_link_libraries(TestLoggerExtention PUBLIC
        queryrunnerimpl
        procimpl
        OsqueryExtensions
        ${testhelperslib}
        ${zmqlib}
       ${log4cpluslib}
       ${protobuflib}
       ${expatlib}
       ${zlib}
       ${JSONCPP_LIBRARY}
       ${GFLAG_LIBRARY}
       ${GLOG_LIBRARY}
       dl
       ${OPENSSL_LIBRARY}
       ${THRIFT_LIBRARY}
        )

target_include_directories(BinaryDataTable PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/modules
        ${OSQUERYSDK}/include
        ${GOOGLETESTINCLUDE}
        ${BOOST_INCLUDE_DIR}
        ${SQLITE_INCLUDE_DIR}
        ${testhelpersinclude}
        )

       SET_TARGET_PROPERTIES(TestLoggerExtention
       PROPERTIES
       BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
       INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
       )

       install(TARGETS TestLoggerExtention
       DESTINATION ../componenttests)