include(GoogleTest)
add_executable(TestEventJournal TestEventJournal.cpp)

target_include_directories(TestEventJournal SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestEventJournal BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" ${testhelpersinclude} ${CMAKE_SOURCE_DIR})
target_link_libraries(TestEventJournal gtest gmock gtest_main ${protobuflib} eventjournal ${log4cpluslib} ${pluginapilib} ${testhelperslib} ${LZMA_LIBRARY} stdc++fs)
gtest_discover_tests(TestEventJournal)

SET_TARGET_PROPERTIES(TestEventJournal
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
