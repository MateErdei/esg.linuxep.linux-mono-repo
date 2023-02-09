add_executable(zipTool zipTool.cpp)

target_include_directories(zipTool PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})

target_link_libraries(zipTool logging ziputilities)

SET_TARGET_PROPERTIES(zipTool PROPERTIES
        BUILD_RPATH "/opt/sophos-spl/base/lib64"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")


INSTALL(TARGETS
        zipTool
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)

add_executable(unzipTool unzipTool.cpp)

target_include_directories(unzipTool PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})

target_link_libraries(unzipTool logging ziputilities)

SET_TARGET_PROPERTIES(unzipTool PROPERTIES
        BUILD_RPATH "/opt/sophos-spl/base/lib64"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64")


INSTALL(TARGETS
        unzipTool
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)