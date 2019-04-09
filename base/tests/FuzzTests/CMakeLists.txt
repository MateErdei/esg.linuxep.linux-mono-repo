
add_executable(loggerconfigtests
        loggerconfigtests.cpp
        $<TARGET_OBJECTS:loggingobject>
        $<TARGET_OBJECTS:filesystemimplobject>
        $<TARGET_OBJECTS:applicationconfigurationimplobject>)

target_include_directories(loggerconfigtests PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})

target_link_libraries(loggerconfigtests PUBLIC
         ${LOG4CPLUS_LIBRARY}
        )

SET_TARGET_PROPERTIES( loggerconfigtests
        PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )
