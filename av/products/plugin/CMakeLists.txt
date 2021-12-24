if (DEFINED ENV{USE_LIBFUZZER})
    return()
endif (DEFINED ENV{USE_LIBFUZZER})

add_executable(${plugin_name}
        main.cpp)
target_link_libraries(${plugin_name} PUBLIC pluginimpl)
target_include_directories(${plugin_name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

SET_TARGET_PROPERTIES( ${plugin_name}
       PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS ${plugin_name}
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE  GROUP_READ GROUP_EXECUTE)

