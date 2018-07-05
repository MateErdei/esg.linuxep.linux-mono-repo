
set(DIST ${CMAKE_BINARY_DIR}/distribution)

set(DISTRIBUTION_LIST
        ${DIST}/install.sh
        ${DIST}/files/base/bin/SulDownloader
        )

set(DISTRIBUTION_LIBRARIES
        modules/Common/DirectoryWatcherImpl/libdirectorywatcherimpl.so
        modules/Common/FileSystemImpl/libfilesystemimpl.so
        modules/Common/ProcessImpl/libprocessimpl.so
        modules/Common/ReactorImpl/libreactorimpl.so
        modules/Common/Threads/libthreads.so
        modules/Common/ZeroMQWrapperImpl/libzeromqwrapperimpl.so
        modules/SulDownloader/libsuldownloaderimpl.so
        )

## Add libraries to DISTRIBUTION_LIST
foreach(lib IN LISTS DISTRIBUTION_LIBRARIES)
    get_filename_component(libbase ${lib} NAME)
    set(lib_target ${DIST}/files/base/lib64/${libbase})
    set(DISTRIBUTION_LIST ${DISTRIBUTION_LIST} ${lib_target})

    add_custom_command(
            OUTPUT ${lib_target}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/${lib}
            ${lib_target}
    )
endforeach(lib)

#file(WRITE DISTRIBUTION_LIST ${CMAKE_BINARY_DIR}/${DISTRIBUTION_LIST})
configure_file(DISTRIBUTION_LIST.in ${CMAKE_BINARY_DIR}/DISTRIBUTION_LIST)

add_custom_command(OUTPUT ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generateSDDSImportAndManifestDat.py ${DIST} ${CMAKE_BINARY_DIR}/DISTRIBUTION_LIST
        DEPENDS ${DISTRIBUTION_LIST}
        )

add_custom_command(
        OUTPUT ${DIST}/install.sh
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/products/installer/install.sh
            ${DIST}/install.sh
        COMMAND chmod 700 ${DIST}/install.sh
        DEPENDS ${CMAKE_SOURCE_DIR}/products/installer/install.sh
)

add_custom_command(
        OUTPUT ${DIST}/files/base/bin/SulDownloader
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/products/SulDownloader/SulDownloader
        ${DIST}/files/base/bin/SulDownloader
        COMMAND chmod 700 ${DIST}/install.sh
        DEPENDS products/SulDownloader/SulDownloader
)

add_custom_target(
        dist ALL
        DEPENDS ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
)
