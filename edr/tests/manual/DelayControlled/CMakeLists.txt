set(GOOGLETESTINCLUDE ${CMAKE_SOURCE_DIR}/tests/googletest/googletest/include)

add_executable(DelayControlledTable
        DelayControlledTable.cpp
        DelayControlledTable.h
        )

target_link_libraries(DelayControlledTable PUBLIC
        ${GLOG_LIBRARY}
        osquerysdk_library
        ${BOOST_CHRONO_LIBRARY}
        ${BOOST_SYSTEM_LIBRARY}
        ${BOOST_THREAD_LIBRARY}
        ${BOOST_FILESYSTEM_LIBRARY}
        ${BOOST_CONTEXT_LIBRARY}
        ${JSONCPP_LIBRARY}
        pthread
        ${CMAKE_DL_LIBS}
        ${GFLAG_LIBRARY}
        sqlite_library
        )

target_include_directories(DelayControlledTable PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${GOOGLETESTINCLUDE}
        ${BOOST_INCLUDE_DIR}
        ${LIVEQUERY_INCLUDE}
        )

SET_TARGET_PROPERTIES(DelayControlledTable
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64"
        )

install(TARGETS DelayControlledTable
        DESTINATION ../componenttests)



