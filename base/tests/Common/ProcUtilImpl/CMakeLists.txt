include(GoogleTest)
add_executable(ProcUtilImplTests TestProcUtilImpl.cpp)
#target_include_directories(ProcUtilImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include")
target_include_directories(ProcUtilImplTests BEFORE PUBLIC "${GTEST_INCLUDE}" ${CMAKE_SOURCE_DIR})
target_link_libraries(ProcUtilImplTests procutilimpl ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} testhelpers)
gtest_discover_tests(ProcUtilImplTests)
SET_TARGET_PROPERTIES(ProcUtilImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
