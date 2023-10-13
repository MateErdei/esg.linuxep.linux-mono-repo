set(GOOGLETESTINCLUDE ${CMAKE_SOURCE_DIR}/tests/googletest/googletest/include)

add_executable(BinaryDataTable
        BinaryDataTable.cpp
        BinaryDataTable.h
        ../../osqueryextensions/MockJournalReaderWrapper.h)

target_link_libraries(BinaryDataTable PUBLIC
        ${GLOG_LIBRARY}
        osquerysdk_library
        ${JSONCPP_LIBRARY}
        pthread
        ${CMAKE_DL_LIBS}
        ${GFLAG_LIBRARY}
        sqlite_library
        )

target_include_directories(BinaryDataTable PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${GOOGLETESTINCLUDE}
        ${BOOST_INCLUDE_DIR}
        ${LIVEQUERY_INCLUDE}
        )

SET_TARGET_PROPERTIES(BinaryDataTable
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64"
        )

install(TARGETS BinaryDataTable
        DESTINATION ../componenttests)
