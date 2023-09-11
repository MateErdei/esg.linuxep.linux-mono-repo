execute_process(
        COMMAND bash "-c" "ldd --version | grep 'ldd (.*)' | rev | cut -d ' ' -f 1 | rev"
        OUTPUT_VARIABLE BUILD_SYSTEM_LIBC_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)

configure_file(install.in.sh install.sh @ONLY)
configure_file(uninstall.in.sh uninstall.sh @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/install.sh
        DESTINATION ${BASE_DIST_PREFIX}
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/products/distribution/cleanupinstall.sh
        DESTINATION ${BASE_DIST_PREFIX}
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/products/distribution/checkAndRunExtraUpgrade.sh
        DESTINATION ${BASE_DIST_PREFIX}
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/products/Base/installer/filestodelete.dat
        DESTINATION ${BASE_DIST_PREFIX}
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/products/Base/installer/cleanuprealm.dat
        DESTINATION ${BASE_DIST_PREFIX}
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/uninstall.sh
        DESTINATION ${BASE_DIST_PREFIX}/files/bin
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install(FILES ${CMAKE_SOURCE_DIR}/products/Base/installer/version
        DESTINATION ${BASE_DIST_PREFIX}/files/bin
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install(FILES ${CMAKE_SOURCE_DIR}/products/Base/installer/logger.conf
        DESTINATION ${BASE_DIST_PREFIX}/files/base/etc
        PERMISSIONS OWNER_READ OWNER_WRITE)

install(FILES ${CMAKE_SOURCE_DIR}/products/Base/Diagnose/DiagnosePaths.conf
        DESTINATION ${BASE_DIST_PREFIX}/files/base/etc
        PERMISSIONS OWNER_READ OWNER_WRITE)

install(FILES ${CMAKE_SOURCE_DIR}/products/Base/installer/downgradepaths.dat
        DESTINATION ${BASE_DIST_PREFIX}/files/base/etc
        PERMISSIONS OWNER_READ OWNER_WRITE)

install(FILES ${CMAKE_SOURCE_DIR}/products/Base/installer/backupfileslist.dat
        DESTINATION ${BASE_DIST_PREFIX}/files/base/etc
        PERMISSIONS OWNER_READ OWNER_WRITE)

install(FILES ${CMAKE_SOURCE_DIR}/products/Base/installer/rootca384.crt
        DESTINATION ${BASE_DIST_PREFIX}/files/base/update/rootcerts
        PERMISSIONS OWNER_READ OWNER_WRITE)

add_executable(versionedcopy
        VersionedCopyMain.cpp
        ${PROJECT_SOURCE_DIR}/modules/Installer/VersionedCopy/VersionedCopy.h
        )

target_link_libraries(versionedcopy PUBLIC versionedcopylib)

SET_TARGET_PROPERTIES( versionedcopy
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")

install(TARGETS versionedcopy
        DESTINATION ${BASE_DIST_PREFIX}/files/base/bin)

add_executable(manifestdiff
         ManifestDiffMain.cpp
        ${PROJECT_SOURCE_DIR}/modules/Installer/ManifestDiff/ManifestDiff.h
        )

add_executable(machineid
        MachineIDMain.cpp
        )


target_link_libraries(manifestdiff PUBLIC manifestdifflib)
target_link_libraries(machineid PUBLIC machineidlib)

SET_TARGET_PROPERTIES( manifestdiff
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")

SET_TARGET_PROPERTIES( machineid
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")


install(TARGETS manifestdiff
        DESTINATION ${BASE_DIST_PREFIX}/files/base/bin)

install(TARGETS machineid
        DESTINATION ${BASE_DIST_PREFIX}/files/base/bin)

install(FILES ${CMAKE_SOURCE_DIR}/products/Base/installer/licenses.txt
        DESTINATION ${BASE_DIST_PREFIX}/files/base/etc
        PERMISSIONS OWNER_READ
        )

install(FILES ${TELEMETRY_CONFIG_FILE}
        DESTINATION ${BASE_DIST_PREFIX}/files/base/etc
        PERMISSIONS OWNER_READ
        )

install ( DIRECTORY ${GCC_INPUT}/lib64
        DESTINATION ${BASE_DIST_PREFIX}/files/base/
        CONFIGURATIONS Debug
        FILES_MATCHING PATTERN libasan.so.*
        )

install ( DIRECTORY ${GCC_INPUT}/lib64
        DESTINATION ${BASE_DIST_PREFIX}/files/base/
        CONFIGURATIONS Debug
        FILES_MATCHING PATTERN libubsan.so.*
        )
