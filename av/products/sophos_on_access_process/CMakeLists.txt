
set(SOAPD_EXE_NAME soapd)

add_executable(${SOAPD_EXE_NAME}
        sophos_on_access_process_main.cpp
        )

target_link_libraries(${SOAPD_EXE_NAME} PUBLIC
        soapdbootstrap
        )
target_include_directories(${SOAPD_EXE_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/modules)

SET_TARGET_PROPERTIES(${SOAPD_EXE_NAME}
        PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS ${SOAPD_EXE_NAME}
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE )
