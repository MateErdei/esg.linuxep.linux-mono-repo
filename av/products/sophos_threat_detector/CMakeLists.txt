
add_executable(sophos_threat_detector
        sophos_threat_detector_main.cpp
        )
target_link_libraries(sophos_threat_detector  PUBLIC sophosthreatdetectorimpl)
target_include_directories(sophos_threat_detector PUBLIC ${CMAKE_SOURCE_DIR}/modules)

SET_TARGET_PROPERTIES( sophos_threat_detector
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:$ORIGIN:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )

install(TARGETS sophos_threat_detector
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        )
