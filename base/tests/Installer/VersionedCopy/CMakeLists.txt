include(GoogleTest)
add_executable(TestVersionedCopy TestVersionedCopy.cpp)
target_include_directories(TestVersionedCopy SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestVersionedCopy BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${BOOST_INCLUDE}" "${CMAKE_SOURCE_DIR}" )

set (BOOST_LIB_DIRS "${BOOST_INPUT}/lib64")

set (CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${BOOST_LIB_DIRS} )


target_link_libraries(TestVersionedCopy gtest gtest_main gmock )
target_link_libraries(TestVersionedCopy versionedcopylib testhelpers -L"${BOOST_LIB_DIRS}" -lboost_filesystem -lboost_system )
gtest_discover_tests(TestVersionedCopy)
