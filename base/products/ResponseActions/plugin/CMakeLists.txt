add_executable(${RA_PLUGIN_NAME}
        main.cpp)

include_directories(${RA_PLUGIN_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/modules/ResponseActions ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(${RA_PLUGIN_NAME} PUBLIC pluginimpl)

SET_TARGET_PROPERTIES( ${RA_PLUGIN_NAME}
       PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS ${RA_PLUGIN_NAME}
        DESTINATION ra/files/plugins/${RA_PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE  GROUP_READ GROUP_EXECUTE)

