set( GOOGLETESTINCLUDE ${CMAKE_SOURCE_DIR}/tests/googletest/googletest/include )

sophos_add_exe(sophos_servertable SHARED
        SophosServerTable.cpp
        EXTRA_INCLUDES ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules ${BOOST_INCLUDE_DIR}
        )

target_link_libraries(sophos_servertable PUBLIC
        ${GLOG_LIBRARY}
        ${GFLAG_LIBRARY}
        ${OSQUERY_LIBRARY}
        ${THRIFT_LIBRARY}
        ${BOOST_CHRONO_LIBRARY}
        ${BOOST_SYSTEM_LIBRARY}
        ${BOOST_THREAD_LIBRARY}
        ${BOOST_FILESYSTEM_LIBRARY}
        ${BOOST_CONTEXT_LIBRARY}
        ${OPENSSLCRYPTO_LIBRARY}
        ${OPENSSL_LIBRARY}

        )

target_include_directories(sophos_servertable PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${OSQUERYSDK}/include
        ${GOOGLETESTINCLUDE}
        ${BOOST_INCLUDE_DIR}
        )

SET_TARGET_PROPERTIES(sophos_servertable
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS sophos_servertable
        DESTINATION files/plugins/${PLUGIN_NAME}/extensions
        PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE)


