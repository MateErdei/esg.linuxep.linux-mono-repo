include(GoogleTest)

add_executable(
        eventtypesimpltestsexe
        TestEventConverterCredentialEvent.cpp
        TestEventConverterPortScanningEvent.cpp
        TestEventTypeHelper.h)

target_include_directories(eventtypesimpltestsexe SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include"
        "${gmock_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/modules/Common/EventTypesImpl/capnproto"
        "${CAPNPROTO_INCLUDE_DIR}"
        ../../../
        )
target_include_directories(eventtypesimpltestsexe BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(eventtypesimpltestsexe gtest gtest_main)
target_link_libraries(eventtypesimpltestsexe eventtypesimpl testhelpers)
message($ENV{LD_LIBRARY_PATH})

gtest_discover_tests(eventtypesimpltestsexe)