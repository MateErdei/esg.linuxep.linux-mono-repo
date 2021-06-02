add_executable(SDU main.cpp)
target_include_directories(SDU PUBLIC ${PROJECT_SOURCE_DIR}/modules/sdu
        )

target_link_libraries(SDU PUBLIC remotediagnoseimpl)

configure_file(sdu.json.in  sdu.json @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdu.json
        DESTINATION installer/plugins
        PERMISSIONS OWNER_READ )

SET_TARGET_PROPERTIES( SDU
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

install(TARGETS SDU
        DESTINATION files/base/bin)