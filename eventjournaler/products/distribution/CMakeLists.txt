

configure_file(PRODUCT_NAME.in PRODUCT_NAME @ONLY )
configure_file(PLUGIN_NAME.in PLUGIN_NAME @ONLY )
configure_file(RIGID_NAME.in RIGID_NAME @ONLY )
configure_file(DEFAULT_HOME_FOLDER.in DEFAULT_HOME_FOLDER @ONLY )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libstdc++.so
        OUTPUT_VARIABLE libstdcpp_path)
string(STRIP "${libstdcpp_path}" libstdcpp_path )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libgcc_s.so.1
        OUTPUT_VARIABLE libgcc_s_path)
string(STRIP "${libgcc_s_path}" libgcc_s_path )

set(third_party_libs ${libstdcpp_path} ${libgcc_s_path} ${pluginapilib} ${zmqlib} ${log4cpluslib} ${protobuflib})

set(DISTRIBUTION_TOOL_DIR ${PLUGINAPIPATH}/distribution)
set(STRIP_EXECUTABLES ${DISTRIBUTION_TOOL_DIR}/stripExecutables.py)
set(ADD_SYMBOLS_SCRIPT ${DISTRIBUTION_TOOL_DIR}/addSymbols.sh)
set(SYMBOLS_DIR ${CMAKE_BINARY_DIR}/symbols)
set(GENERATE_SDDS_IMPORT_MANIFEST_DAT ${DISTRIBUTION_TOOL_DIR}/generateSDDSImportAndManifestDat.py )

add_custom_command(OUTPUT ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
        COMMAND python ${STRIP_EXECUTABLES} ${DIST} ${SYMBOLS_DIR}  ${CMAKE_BUILD_TYPE}
        COMMAND ${CMAKE_COMMAND} -E copy ${ADD_SYMBOLS_SCRIPT} ${SYMBOLS_DIR}/
        COMMAND python ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST} ${CMAKE_BUILD_TYPE}
        )

add_custom_target(
        dist
        COMMAND python ${STRIP_EXECUTABLES} ${DIST} ${SYMBOLS_DIR}  ${CMAKE_BUILD_TYPE}
        COMMAND ${CMAKE_COMMAND} -E copy ${ADD_SYMBOLS_SCRIPT} ${SYMBOLS_DIR}/
        COMMAND python ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST} ${CMAKE_BUILD_TYPE}
)

add_custom_target(
        forcedist
        COMMAND rm -f ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
        COMMAND python ${STRIP_EXECUTABLES} ${DIST} ${SYMBOLS_DIR}  ${CMAKE_BUILD_TYPE}
        COMMAND ${CMAKE_COMMAND} -E copy ${ADD_SYMBOLS_SCRIPT} ${SYMBOLS_DIR}/
        COMMAND python ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST} ${CMAKE_BUILD_TYPE}
)

install(FILES  ${third_party_libs}
        DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

add_custom_target(
        copy_libs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/libs
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/copy_libs.py ${CMAKE_BINARY_DIR}/libs ${third_party_libs}
)