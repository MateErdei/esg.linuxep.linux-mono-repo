set(GOOGLETESTINCLUDE ${CMAKE_SOURCE_DIR}/tests/googletest/googletest/include)

add_executable(MemoryCrashTable
        MemoryCrashTable.cpp
        MemoryCrashTable.h
        )

target_link_libraries(MemoryCrashTable PUBLIC
        gflags
        osquerysdk_library
        ${JSONCPP_LIBRARY}
        pthread
        ${CMAKE_DL_LIBS}
        sqlite_library
        )

target_include_directories(MemoryCrashTable PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${GOOGLETESTINCLUDE}
        ${BOOST_INCLUDE_DIR}
        ${LIVEQUERY_INCLUDE}
        )

SET_TARGET_PROPERTIES(MemoryCrashTable
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64"
        )

install(TARGETS MemoryCrashTable
        DESTINATION ../componenttests)



