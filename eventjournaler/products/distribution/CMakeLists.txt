

configure_file(PRODUCT_NAME.in PRODUCT_NAME @ONLY )
configure_file(PLUGIN_NAME.in PLUGIN_NAME @ONLY )
configure_file(RIGID_NAME.in RIGID_NAME @ONLY )
configure_file(DEFAULT_HOME_FOLDER.in DEFAULT_HOME_FOLDER @ONLY )

set(DISTRIBUTION_TOOL_DIR ${PLUGINAPIPATH}/distribution)
set(STRIP_EXECUTABLES ${DISTRIBUTION_TOOL_DIR}/stripExecutables.py)
set(GENERATE_SDDS_IMPORT_MANIFEST_DAT ${DISTRIBUTION_TOOL_DIR}/generateSDDSImportAndManifestDat.py )

add_custom_command(OUTPUT ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
        COMMAND python ${STRIP_EXECUTABLES} ${DIST} ${CMAKE_BINARY_DIR}/symbols ${CMAKE_BUILD_TYPE}
        COMMAND python ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST} ${CMAKE_BUILD_TYPE}
        )

add_custom_target(
        dist
        COMMAND python ${STRIP_EXECUTABLES} ${DIST} ${CMAKE_BINARY_DIR}/symbols ${CMAKE_BUILD_TYPE}
        COMMAND python ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST} ${CMAKE_BUILD_TYPE}
)

add_custom_target(
        forcedist
        COMMAND rm -f ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
        COMMAND python ${STRIP_EXECUTABLES} ${DIST} ${CMAKE_BINARY_DIR}/symbols ${CMAKE_BUILD_TYPE}
        COMMAND python ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST} ${CMAKE_BUILD_TYPE}
)

install(FILES  ${pluginapilib} ${zmqlib} ${log4cpluslib}
        DESTINATION files/plugins/${PLUGIN_NAME}/lib64)


