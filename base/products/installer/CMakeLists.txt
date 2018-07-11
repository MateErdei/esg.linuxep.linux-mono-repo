
install(FILES ${CMAKE_SOURCE_DIR}/products/installer/install.sh
        DESTINATION .
        PERMISSIONS OWNER_READ OWNER_EXECUTE
        )

add_executable(versionedcopy
        VersionedCopyMain.cpp ../../modules/Installer/VersionedCopy/VersionedCopy.h)


target_link_libraries(versionedcopy PUBLIC versionedcopylib)

install(TARGETS versionedcopy
        DESTINATION installer)

