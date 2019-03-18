
install(FILES ${CMAKE_SOURCE_DIR}/products/installer/install.sh
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/products/installer/uninstall.sh
        DESTINATION files/bin
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

install(FILES ${CMAKE_SOURCE_DIR}/products/installer/logger.conf
        DESTINATION files/base/etc
        PERMISSIONS OWNER_READ OWNER_WRITE)

install(FILES ${CMAKE_SOURCE_DIR}/products/Diagnose/DiagnoseLogFilePaths.conf
        DESTINATION files/base/etc
        PERMISSIONS OWNER_READ OWNER_WRITE)

install(FILES ${CMAKE_SOURCE_DIR}/products/Diagnose/DiagnoseMCSDirectoryPaths.conf
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

install(TARGETS manifestdiff
        DESTINATION files/base/bin)

install(TARGETS machineid
        DESTINATION files/base/bin)
