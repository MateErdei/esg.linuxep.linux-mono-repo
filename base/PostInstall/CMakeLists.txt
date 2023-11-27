set(STRIP_EXECUTABLES ${DISTRIBUTION_TOOL_DIR}/stripExecutables.py)
set(ADD_SYMBOLS_SCRIPT ${DISTRIBUTION_TOOL_DIR}/addSymbols.sh)
set(SYMBOLS_DIR ${CMAKE_BINARY_DIR}/symbols)
set(GENERATE_SDDS_IMPORT_MANIFEST_DAT ${DISTRIBUTION_TOOL_DIR}/generateSDDSImportAndManifestDat.py)

INSTALL(CODE "MESSAGE(STATUS \"Post install steps\")")

INSTALL(CODE "MESSAGE(STATUS \"Taring Thininstaller Dependencies\")
    execute_process(
        COMMAND tar czf ${CMAKE_BINARY_DIR}/products/ThininstallerDependencies/cmcsrouterapi.tar.gz -C ${CMAKE_BINARY_DIR}/products/ThininstallerDependencies/  mcslibrary
        COMMAND_ERROR_IS_FATAL ANY)")

INSTALL(CODE "MESSAGE(STATUS \"Cleaning Thininstaller Dependencies dir\")
    execute_process(
        COMMAND rm -rf ${CMAKE_BINARY_DIR}/products/ThininstallerDependencies/mcslibrary
        COMMAND_ERROR_IS_FATAL ANY)")

# Strip executables
INSTALL(CODE "MESSAGE(STATUS \"Stripping executables\")
    execute_process(
        COMMAND python3 ${STRIP_EXECUTABLES} ${DIST} ${SYMBOLS_DIR} ${CMAKE_BUILD_TYPE}
        RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
      message(STATUS \"Failed to strip executables: ${STATUS}\")
    endif()")

# Copy addSymbol script to symbol dir
INSTALL(CODE "MESSAGE(STATUS \"Copying addSymbol script to ${SYMBOLS_DIR}\")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy ${ADD_SYMBOLS_SCRIPT} ${SYMBOLS_DIR}/base
        RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
        message(STATUS \"Failed to copy symbols to output: ${STATUS}\")
    endif()")

# Generate SDDS manifests
INSTALL(CODE "MESSAGE(STATUS \"Generating base SDDS manifests\")
    set(ENV{PRODUCT_NAME} \"${PRODUCT_NAME}\")
    set(ENV{PRODUCT_LINE_ID} \"${PRODUCT_LINE_ID}\")
    set(ENV{DEFAULT_HOME_FOLDER} \"${DEFAULT_HOME_FOLDER}\")
    set(ENV{FEATURE_LIST} \"${FEATURE_LIST}\")
    set(ENV{VERSION} \"${PRODUCT_VERSION}\")
    execute_process(
        COMMAND python3 ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST}/${BASE_DIST_PREFIX} ${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
        message(FATAL_ERROR \"Failed to generate SDDS manifests: ${STATUS}\")
    endif()")

INSTALL(CODE "MESSAGE(STATUS \"Generating ra SDDS manifests\")
    set(ENV{PRODUCT_NAME} \"${RA_PRODUCT_NAME}\")
    set(ENV{PRODUCT_LINE_ID} \"${RA_PRODUCT_LINE_ID}\")
    set(ENV{DEFAULT_HOME_FOLDER} \"${DEFAULT_HOME_FOLDER}\")
    set(ENV{FEATURE_LIST} \"${RA_FEATURE_LIST}\")
    set(ENV{VERSION} \"${RA_PRODUCT_VERSION}\")
    execute_process(
        COMMAND python3 ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST}/${RA_DIST_PREFIX} ${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
        message(FATAL_ERROR \"Failed to generate SDDS manifests: ${STATUS}\")
    endif()")


# Generate the output dir to be published
INSTALL(CODE "MESSAGE(STATUS \"Generating output directory\")
    execute_process(COMMAND mkdir -p ${OUTPUT})
    execute_process(COMMAND rm -rf ${OUTPUT}/SDDS-COMPONENT)
    execute_process(COMMAND cp -a ${DIST}/${BASE_DIST_PREFIX} ${OUTPUT}/SDDS-COMPONENT)
    execute_process(COMMAND rm -rf ${OUTPUT}/RA-SDDS-COMPONENT)
    execute_process(COMMAND cp -a ${DIST}/${RA_DIST_PREFIX} ${OUTPUT}/RA-SDDS-COMPONENT)
    execute_process(COMMAND cp -a ${CMAKE_BINARY_DIR}/products/PluginApi/pluginapi.tar.gz ${OUTPUT}/pluginapi.tar.gz)
    execute_process(COMMAND tar -zcvf ${OUTPUT}/SystemProductTestOutput.tar.gz -C ${CMAKE_BINARY_DIR} SystemProductTestOutput/)
    execute_process(COMMAND bash -c \"[[ -d ${CMAKE_BINARY_DIR}/symbols/base ]] && cp -a ${CMAKE_BINARY_DIR}/symbols/base ${OUTPUT}/base-symbols && echo copied base symbols to output \")
    execute_process(COMMAND bash -c \"[[ -d ${CMAKE_BINARY_DIR}/symbols/ra ]] && cp -a ${CMAKE_BINARY_DIR}/symbols/ra ${OUTPUT}/ra-symbols && echo copied ra symbols to output \")
    execute_process(COMMAND cp -a ${CMAKE_BINARY_DIR}/products/ThininstallerDependencies/cmcsrouterapi.tar.gz ${OUTPUT}/cmcsrouterapi.tar.gz)
")