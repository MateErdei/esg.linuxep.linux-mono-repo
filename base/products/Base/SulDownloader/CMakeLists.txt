message("directory : " ${PROJECT_BINARY_DIR}/modules/SulDownloader)
add_executable(SulDownloader main.cpp)
target_include_directories(SulDownloader  PUBLIC ${PROJECT_SOURCE_DIR}/modules/SulDownloader)

target_link_libraries(SulDownloader suldownloaderimpl_static)

SET_TARGET_PROPERTIES( SulDownloader
        PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )


install(TARGETS SulDownloader
        DESTINATION ${BASE_DIST_PREFIX}/files/base/bin)

# Export suldownloader for thin installer as well.
install(TARGETS SulDownloader DESTINATION ${THININSTALLER_DEPS_DIR_BIN})

add_dependencies(SulDownloader copy_libs)

