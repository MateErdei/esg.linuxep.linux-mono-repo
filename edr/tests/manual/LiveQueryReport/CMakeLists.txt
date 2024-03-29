set( GOOGLETESTINCLUDE ${CMAKE_SOURCE_DIR}/tests/googletest/googletest/include )

add_executable(LiveQueryReport
        LiveQueryReport.cpp
        )

target_link_libraries(LiveQueryReport PUBLIC osqueryclient
        ${GLOG_LIBRARY}
        ${GFLAG_LIBRARY}
        ${zmqlib}
        ${log4cpluslib}
        ${protobuflib}
        ${JSONCPP_LIBRARY}
        pthread
        ${CMAKE_DL_LIBS}
        sqlite_library
        )

target_include_directories(LiveQueryReport PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/modules
        ${GOOGLETESTINCLUDE}
        ${BOOST_INCLUDE_DIR}
        )

SET_TARGET_PROPERTIES( LiveQueryReport
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64"
        )

install(TARGETS LiveQueryReport
        DESTINATION ../componenttests)
