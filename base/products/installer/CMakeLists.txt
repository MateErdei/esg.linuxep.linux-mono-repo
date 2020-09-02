execute_process(
        COMMAND bash "-c" "ldd --version | grep 'ldd (.*)' | rev | cut -d ' ' -f 1 | rev"
        OUTPUT_VARIABLE BUILD_SYSTEM_LIBC_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)

configure_file(install.in.sh install.sh @ONLY)
configure_file(uninstall.in.sh uninstall.sh @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/install.sh
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/products/distribution/cleanupinstall.sh
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/products/installer/filestodelete.dat
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/products/installer/cleanuprealm.dat
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/uninstall.sh
        DESTINATION files/bin
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install(FILES ${CMAKE_SOURCE_DIR}/products/installer/version
        DESTINATION files/bin
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install(FILES ${CMAKE_SOURCE_DIR}/products/installer/logger.conf
        DESTINATION files/base/etc
        PERMISSIONS OWNER_READ OWNER_WRITE)

install(FILES ${CMAKE_SOURCE_DIR}/products/Diagnose/DiagnosePaths.conf
        DESTINATION files/base/etc
        PERMISSIONS OWNER_READ OWNER_WRITE)

install(FILES ${CMAKE_SOURCE_DIR}/products/installer/DowngradePaths.conf
        DESTINATION files/base/etc
        PERMISSIONS OWNER_READ OWNER_WRITE)

add_executable(versionedcopy
        VersionedCopyMain.cpp
        ../../modules/Installer/VersionedCopy/VersionedCopy.h
        )

target_link_libraries(versionedcopy PUBLIC versionedcopylib)

SET_TARGET_PROPERTIES( versionedcopy
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")

install(TARGETS versionedcopy
        DESTINATION files/base/bin)

add_executable(manifestdiff
         ManifestDiffMain.cpp
        ../../modules/Installer/ManifestDiff/ManifestDiff.h
        )

add_executable(machineid
        MachineIDMain.cpp
        )


target_link_libraries(manifestdiff PUBLIC manifestdifflib)
target_link_libraries(machineid PUBLIC osutilitiesimpl)

SET_TARGET_PROPERTIES( manifestdiff
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")

SET_TARGET_PROPERTIES( machineid
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")


install(TARGETS manifestdiff
        DESTINATION files/base/bin)

install(TARGETS machineid
        DESTINATION files/base/bin)

install(FILES ${CMAKE_SOURCE_DIR}/products/installer/licenses.txt
        DESTINATION files/base/etc
        PERMISSIONS OWNER_READ
        )

install(FILES ${TELEMETRY_CONFIG_FILE}
        DESTINATION files/base/etc
        PERMISSIONS OWNER_READ
        )

install ( DIRECTORY ${GCC_INPUT}/lib64
        DESTINATION files/base/
        CONFIGURATIONS Debug
        FILES_MATCHING PATTERN libasan.so.*
        )

install ( DIRECTORY ${GCC_INPUT}/lib64
        DESTINATION files/base/
        CONFIGURATIONS Debug
        FILES_MATCHING PATTERN libubsan.so.*
        )
