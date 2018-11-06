

configure_file(PLUGIN_NAME.in PLUGIN_NAME @ONLY )
configure_file(RIGID_NAME.in RIGID_NAME @ONLY )

add_custom_command(OUTPUT ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generateSDDSImportAndManifestDat.py ${DIST}
        )

add_custom_target(
        dist
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generateSDDSImportAndManifestDat.py ${DIST}
)


add_custom_target(
        forcedist
        COMMAND rm -f ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generateSDDSImportAndManifestDat.py ${DIST}
)

install(FILES  ${pluginapilib} ${zmqlib} ${log4cpluslib}
        DESTINATION files/plugins/${PLUGIN_NAME}/lib64)


