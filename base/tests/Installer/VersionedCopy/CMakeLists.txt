include(GoogleTest)
add_executable(TestVersionedCopy TestVersionedCopy.cpp)
target_include_directories(TestVersionedCopy SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" "${BOOST_INCLUDE}" )
target_include_directories(TestVersionedCopy BEFORE PUBLIC "${CMAKE_SOURCE_DIR}" )

set (BOOST_LIB_DIRS "${BOOST_INPUT}/lib64")
set (TestVersionedCopy_RPATH "${CMAKE_BINARY_DIR}/libs:${BOOST_LIB_DIRS}" )

SET_TARGET_PROPERTIES(TestVersionedCopy
        PROPERTIES
        INSTALL_RPATH "${TestVersionedCopy_RPATH}"
        BUILD_RPATH "${TestVersionedCopy_RPATH}"
        )

target_link_libraries(TestVersionedCopy ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
target_link_libraries(TestVersionedCopy versionedcopylib testhelpers -L"${BOOST_LIB_DIRS}" -lboost_filesystem -lboost_system )
gtest_discover_tests(TestVersionedCopy)

add_dependencies(TestVersionedCopy
        copy_libs
        )
