

configure_file(PRODUCT_NAME.in PRODUCT_NAME @ONLY )
configure_file(PLUGIN_NAME.in PLUGIN_NAME @ONLY )
configure_file(RIGID_NAME.in RIGID_NAME @ONLY )
configure_file(DEFAULT_HOME_FOLDER.in DEFAULT_HOME_FOLDER @ONLY )
configure_file(FEATURE_LIST.in FEATURE_LIST @ONLY )
set( GCC_PRINT ${CMAKE_MODULE_PATH}/gcc-print-file-name.sh)
execute_process(COMMAND bash ${GCC_PRINT} libstdc++.so
        OUTPUT_VARIABLE libstdcpp_path)
string(STRIP "${libstdcpp_path}" libstdcpp_path )

execute_process(COMMAND bash ${GCC_PRINT} libgcc_s.so.1
        OUTPUT_VARIABLE libgcc_s_path)
string(STRIP "${libgcc_s_path}" libgcc_s_path )

execute_process(COMMAND bash ${GCC_PRINT} libatomic.so.1
        OUTPUT_VARIABLE libatomic_path)
string(STRIP "${libatomic_path}" libatomic_path )

set(third_party_libs
        ${libstdcpp_path}
        ${libgcc_s_path}
        ${libatomic_path}
        ${pluginapilib}
        ${expatlib}
        ${zmqlib}
        ${log4cpluslib}
        ${protobuflib}
        ${REDIST}/openssl/lib64/libssl.so.3
        ${REDIST}/openssl/lib64/libcrypto.so.3
        )

INCLUDE(FindLibraryFullPath)

set(DISTRIBUTION_TOOL_DIR ${PLUGINAPIPATH}/distribution)
set(STRIP_EXECUTABLES ${DISTRIBUTION_TOOL_DIR}/stripExecutables.py)
set(ADD_SYMBOLS_SCRIPT ${DISTRIBUTION_TOOL_DIR}/addSymbols.sh)
set(SYMBOLS_DIR ${CMAKE_BINARY_DIR}/symbols)
set(GENERATE_SDDS_IMPORT_MANIFEST_DAT ${DISTRIBUTION_TOOL_DIR}/generateSDDSImportAndManifestDat.py)

add_custom_target(
        dist
        COMMAND python3 ${STRIP_EXECUTABLES} ${DIST} ${SYMBOLS_DIR}  ${CMAKE_BUILD_TYPE}
        COMMAND ${CMAKE_COMMAND} -E copy ${ADD_SYMBOLS_SCRIPT} ${SYMBOLS_DIR}/
        COMMAND python3 ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST} ${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR}
)

install(FILES  ${third_party_libs}
        DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

add_custom_target(
        copy_libs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/libs
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/copy_libs.py ${CMAKE_BINARY_DIR}/libs ${third_party_libs}
)
if (DEFINED ENV{VERSION_OVERRIDE})
    set(PRODUCT_VERSION "$ENV{VERSION_OVERRIDE}")
else()
    execute_process(
            COMMAND python3 ${PLUGINAPIPATH}/distribution/getReleasePackageVersion.py ${RELEASE_PACKAGE_PATH}
            OUTPUT_VARIABLE RELEASE_PACKAGE_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(
            COMMAND python3 ${PLUGINAPIPATH}/distribution/computeAutoVersion.py ${CMAKE_SOURCE_DIR} ${RELEASE_PACKAGE_VERSION}
            OUTPUT_VARIABLE PRODUCT_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

execute_process(
        COMMAND date +%Y-%m-%d
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE BUILD_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
        COMMAND cat ${PLUGINAPIPATH}/PluginAPICommitHash
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE PLUGIN_API_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

configure_file(VERSION.ini.in VERSION.ini @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VERSION.ini
        DESTINATION files/plugins/${PLUGIN_NAME}/)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VERSION.ini
        DESTINATION .)