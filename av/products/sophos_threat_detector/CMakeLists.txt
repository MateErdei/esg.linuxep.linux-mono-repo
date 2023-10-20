add_executable(sophos_threat_detector
        sophos_threat_detector_inner_main.cpp
        sophos_threat_detector_inner_main.h
        sophos_threat_detector_main.cpp
        LogSetup.cpp
        LogSetup.h
)
target_link_libraries(sophos_threat_detector  PUBLIC sophosthreatdetectorimpl ${log4cpluslib})
target_include_directories(sophos_threat_detector PUBLIC ${CMAKE_SOURCE_DIR}/modules)

SET_TARGET_PROPERTIES( sophos_threat_detector
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN/../chroot/lib64:$ORIGIN/../lib64"
        )

add_executable(sophos_threat_detector_launcher
        sophos_threat_detector_launcher.c
        ../capability/PassOnCapability.c)
target_link_libraries(sophos_threat_detector_launcher  PUBLIC cap)
target_include_directories(sophos_threat_detector_launcher PUBLIC ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR})

install(TARGETS sophos_threat_detector sophos_threat_detector_launcher
        DESTINATION files/plugins/${PLUGIN_NAME}/sbin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        )
