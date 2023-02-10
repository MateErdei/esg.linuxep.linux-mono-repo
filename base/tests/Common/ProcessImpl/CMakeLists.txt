include(GoogleTest)
add_executable(ProcessImplTests TestProcessImpl.cpp ../Helpers/MockProcess.h TestProcessInfo.cpp TestProcImplAndPyP.cpp)
target_include_directories(ProcessImplTests SYSTEM BEFORE PUBLIC
        ${GTEST_INCLUDE}
        ${GMOCK_INCLUDE}
        ${CMAKE_SOURCE_DIR}
        ${BOOST_INCLUDE_DIR})
target_link_libraries(ProcessImplTests ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
target_link_libraries(ProcessImplTests processimpl threads testhelpers)
target_compile_options(ProcessImplTests PRIVATE -DOutDir="${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(ProcessImplTests PickYourPoison)
gtest_discover_tests(ProcessImplTests)
SET_TARGET_PROPERTIES(ProcessImplTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )