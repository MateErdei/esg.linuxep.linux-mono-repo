
set(SAFESTORE_EXE_NAME safestore)

add_executable(${SAFESTORE_EXE_NAME}
        safestore_main.cpp
        )

target_link_libraries(${SAFESTORE_EXE_NAME} PUBLIC
        safestoreimpl
        ${SAFESTORE_LIB}
        )
target_include_directories(${SAFESTORE_EXE_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/modules)
message("asdasdasd")
message(${CMAKE_BINARY_DIR}/libs)
SET_TARGET_PROPERTIES(${SAFESTORE_EXE_NAME}
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS ${SAFESTORE_EXE_NAME}
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
