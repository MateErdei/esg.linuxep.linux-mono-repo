

configure_file(PRODUCT_NAME.in PRODUCT_NAME @ONLY )
configure_file(PLUGIN_NAME.in RA_PLUGIN_NAME @ONLY )
configure_file(RIGID_NAME.in RIGID_NAME @ONLY )
configure_file(DEFAULT_HOME_FOLDER.in DEFAULT_HOME_FOLDER @ONLY )
configure_file(FEATURE_LIST.in FEATURE_LIST @ONLY )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libstdc++.so
        OUTPUT_VARIABLE libstdcpp_path)
string(STRIP "${libstdcpp_path}" libstdcpp_path )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libgcc_s.so.1
        OUTPUT_VARIABLE libgcc_s_path)
string(STRIP "${libgcc_s_path}" libgcc_s_path )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libatomic.so.1
        OUTPUT_VARIABLE libatomic_path)
string(STRIP "${libatomic_path}" libatomic_path )

set(third_party_libs ${libstdcpp_path} ${libgcc_s_path} ${libatomic_path} ${pluginapilib} ${zmqlib} ${log4cpluslib} ${protobuflib})

set(STRIP_EXECUTABLES ${DISTRIBUTION_TOOL_DIR}/stripExecutables.py)
set(ADD_SYMBOLS_SCRIPT ${DISTRIBUTION_TOOL_DIR}/addSymbols.sh)
set(SYMBOLS_DIR ${CMAKE_BINARY_DIR}/symbols)
set(GENERATE_SDDS_IMPORT_MANIFEST_DAT ${DISTRIBUTION_TOOL_DIR}/generateSDDSImportAndManifestDat.py )

add_custom_target(
        dist
        COMMAND python3 ${STRIP_EXECUTABLES} ${DIST}/ra ${SYMBOLS_DIR}  ${CMAKE_BUILD_TYPE}
        COMMAND ${CMAKE_COMMAND} -E copy ${ADD_SYMBOLS_SCRIPT} ${SYMBOLS_DIR}/
        COMMAND python3 ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST}/ra ${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR}
        COMMAND ${INPUT}/sdds3/sdds3-builder --build-package --package-dir ${CMAKE_BINARY_DIR}/SDDS3-PACKAGE --sdds-import ${DIST}/ra/SDDS-Import.xml --nonce `sha256sum ${DIST}/SDDS-Import.xml | head -c 10`
)

#install(FILES  ${third_party_libs}
#        DESTINATION files/plugins/${PLUGIN_NAME}/lib64)


if (DEFINED ENV{VERSION_OVERRIDE})
    set(PRODUCT_VERSION "$ENV{VERSION_OVERRIDE}")
else()
    execute_process(
            COMMAND python3 ${DISTRIBUTION_TOOL_DIR}/getReleasePackageVersion.py ${CMAKE_SOURCE_DIR}/build-files/release-package.xml
            OUTPUT_VARIABLE RELEASE_PACKAGE_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(
            COMMAND python3 ${DISTRIBUTION_TOOL_DIR}/computeAutoVersion.py ${CMAKE_SOURCE_DIR} ${RELEASE_PACKAGE_VERSION} Jenkinsfile
            OUTPUT_VARIABLE PRODUCT_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

execute_process(
        COMMAND date +%Y-%m-%d
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE BUILD_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
        COMMAND cat ${CMAKE_SOURCE_DIR}/products/PluginApi/PluginAPICommitHash
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE PLUGIN_API_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

configure_file(VERSION.ini.in VERSION.ini @ONLY)

install(FILES ${CMAKE_SOURCE_DIR}/products/ResponseActions/distribution/VERSION.ini
        DESTINATION ra/files/plugins/${PLUGIN_NAME}/)

install(FILES ${CMAKE_SOURCE_DIR}/products/ResponseActions/distribution/VERSION.ini
        DESTINATION ra)