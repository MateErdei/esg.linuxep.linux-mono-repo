include(GoogleTest)
add_executable(TestMachineId TestSXLMachineID.cpp)
target_include_directories(TestMachineId SYSTEM BEFORE PUBLIC "${GTEST_INCLUDE}" "${GMOCK_INCLUDE}" "${BOOST_INCLUDE}" )
target_include_directories(TestMachineId BEFORE PUBLIC "${CMAKE_SOURCE_DIR}" )


set(TestMachineId_RPATH "${CMAKE_BINARY_DIR}/libs:${BOOST_LIB_DIRS}")

SET_TARGET_PROPERTIES(TestMachineId
        PROPERTIES
        INSTALL_RPATH "${TestMachineId_RPATH}"
        BUILD_RPATH "${TestMachineId_RPATH}"
        )

target_link_libraries(TestMachineId ${GTEST_MAIN_LIBRARY})
target_link_libraries(TestMachineId versionedcopylib testhelpers)
gtest_discover_tests(TestMachineId)

add_dependencies(TestMachineId
        copy_libs
        )
