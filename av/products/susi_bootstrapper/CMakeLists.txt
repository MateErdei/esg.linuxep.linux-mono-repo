find_library(SUSI_LIB NAMES susi PATHS ${SUSIPATH})

get_filename_component(ABS_SUSI_LIB ${SUSI_LIB} REALPATH)

add_executable(susi_bootstrapper
        susi_bootstrapper_main.cpp)
target_link_libraries(susi_bootstrapper
        PUBLIC ${STD_FILESYSTEM_IF_REQUIRED} ${pluginapilib} ${log4cpluslib} ${SUSI_LIB})
target_include_directories(susi_bootstrapper
        PUBLIC ${CMAKE_BINARY_DIR}/modules ${CMAKE_SOURCE_DIR}/modules ${pluginapiinclude} ${susi_include})

SET_TARGET_PROPERTIES( susi_bootstrapper
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS susi_bootstrapper
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        )
