
install(FILES ${CMAKE_SOURCE_DIR}/products/installer/install.sh
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

install(FILES ${CMAKE_SOURCE_DIR}/products/installer/uninstall.sh
        DESTINATION files/bin
        PERMISSIONS OWNER_READ OWNER_EXECUTE)

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

target_link_libraries(manifestdiff PUBLIC manifestdifflib)

SET_TARGET_PROPERTIES( manifestdiff
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")

install(TARGETS manifestdiff
        DESTINATION files/base/bin)
