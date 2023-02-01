

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



#install(FILES  ${third_party_libs}
#        DESTINATION files/plugins/${PLUGIN_NAME}/lib64)


if (DEFINED ENV{RA_VERSION_OVERRIDE})
    set(RA_PRODUCT_VERSION "$ENV{RA_VERSION_OVERRIDE}")
    message("LOOK HERE1")
    message("version ${RA_PRODUCT_VERSION}")
else()
    execute_process(
            COMMAND versioning_client --component responseactions --version_number 1.0.0
            OUTPUT_VARIABLE RA_PRODUCT_VERSION
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

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VERSION.ini
        DESTINATION ra/files/plugins/${RA_PLUGIN_NAME}/)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VERSION.ini
        DESTINATION ra)