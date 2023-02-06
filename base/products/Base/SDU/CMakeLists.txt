add_executable(sdu main.cpp)
target_include_directories(sdu PUBLIC ${PROJECT_SOURCE_DIR}/modules/sdu
        )

target_link_libraries(sdu PUBLIC remotediagnoseimpl)

configure_file(sdu.json.in  sdu.json @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdu.json
        DESTINATION ${BASE_DIST_PREFIX}/installer/plugins
        PERMISSIONS OWNER_READ )

SET_TARGET_PROPERTIES( sdu
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

install(TARGETS sdu
        DESTINATION ${BASE_DIST_PREFIX}/files/base/bin)