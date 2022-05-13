include(GoogleTest)
add_executable(TestEventJournal TestEventJournal.cpp)

target_include_directories(TestEventJournal SYSTEM BEFORE PUBLIC ${GMOCK_INCLUDE} ${GTEST_INCLUDE})
target_include_directories(TestEventJournal BEFORE PUBLIC ${testhelpersinclude} ${CMAKE_SOURCE_DIR})
target_link_libraries(TestEventJournal ${GTEST_LIBRARY} ${GMOCK_LIBRARY} ${GTEST_MAIN_LIBRARY} ${protobuflib} eventjournal ${log4cpluslib} ${pluginapilib} ${testhelperslib} ${LZMA_LIBRARY} stdc++fs pthread)
gtest_discover_tests(TestEventJournal)

SET_TARGET_PROPERTIES(TestEventJournal
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
