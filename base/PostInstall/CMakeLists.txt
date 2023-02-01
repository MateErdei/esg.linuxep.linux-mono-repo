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
        COMMAND ${CMAKE_COMMAND} -E copy ${ADD_SYMBOLS_SCRIPT} ${SYMBOLS_DIR}/
        RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
        message(STATUS \"Failed to copy symbols to output: ${STATUS}\")
    endif()")

# Generate SDDS2 manifests
INSTALL(CODE "MESSAGE(STATUS \"Generating base SDDS2 manifests\")
    set(ENV{PRODUCT_NAME} \"${PRODUCT_NAME}\")
    set(ENV{PRODUCT_LINE_ID} \"${PRODUCT_LINE_ID}\")
    set(ENV{DEFAULT_HOME_FOLDER} \"${DEFAULT_HOME_FOLDER}\")
    set(ENV{FEATURE_LIST} \"${FEATURE_LIST}\")
    set(ENV{VERSION} \"${PRODUCT_VERSION}\")
    execute_process(
        COMMAND python3 ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST}/base ${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
        message(FATAL_ERROR \"Failed to generate SDDS2 manifests: ${STATUS}\")
    endif()")

INSTALL(CODE "MESSAGE(STATUS \"Generating ra SDDS2 manifests\")
    set(ENV{PRODUCT_NAME} \"${RA_PRODUCT_NAME}\")
    set(ENV{PRODUCT_LINE_ID} \"${RA_PRODUCT_LINE_ID}\")
    set(ENV{DEFAULT_HOME_FOLDER} \"${DEFAULT_HOME_FOLDER}\")
    set(ENV{FEATURE_LIST} \"${RA_FEATURE_LIST}\")
    set(ENV{VERSION} \"${RA_PRODUCT_VERSION}\")
    execute_process(
        COMMAND python3 ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST}/ra ${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
        message(FATAL_ERROR \"Failed to generate SDDS2 manifests: ${STATUS}\")
    endif()")

# Generate SDDS3 manifests
if (NOT EXISTS ${INPUT}/sdds3/sdds3-builder)
    message(FATAL_ERROR "Could not find SDDS3 Builder: ${INPUT}/sdds3/sdds3-builder")
endif()
INSTALL(CODE "MESSAGE(STATUS \"Generating base SDDS3 manifests\")
    execute_process(
        COMMAND rm -rf ${CMAKE_BINARY_DIR}/SDDS3-PACKAGE
        COMMAND ${CMAKE_SOURCE_DIR}/PostInstall/RunSdds3Builder.sh ${INPUT}/sdds3/sdds3-builder ${CMAKE_BINARY_DIR}/SDDS3-PACKAGE ${DIST}/base/SDDS-Import.xml
        RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
        message(FATAL_ERROR \"Failed to generate SDDS3 manifests: ${STATUS}\")
    endif()")

INSTALL(CODE "MESSAGE(STATUS \"Generating ra SDDS3 manifests\")
    execute_process(
        COMMAND rm -rf ${CMAKE_BINARY_DIR}/RA-SDDS3-PACKAGE
        COMMAND ${CMAKE_SOURCE_DIR}/PostInstall/RunSdds3Builder.sh ${INPUT}/sdds3/sdds3-builder ${CMAKE_BINARY_DIR}/RA-SDDS3-PACKAGE ${DIST}/ra/SDDS-Import.xml
        RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
        message(FATAL_ERROR \"Failed to generate SDDS3 manifests: ${STATUS}\")
    endif()")

# Generate the output dir to be published
INSTALL(CODE "MESSAGE(STATUS \"Generating output directory\")
    execute_process(COMMAND mkdir -p ${OUTPUT})
    execute_process(COMMAND rm -rf ${OUTPUT}/SDDS-COMPONENT)
    execute_process(COMMAND rm -rf ${OUTPUT}/SDDS3-COMPONENT)
    execute_process(COMMAND cp -a ${DIST}/base ${OUTPUT}/SDDS-COMPONENT)
    execute_process(COMMAND cp -a ${CMAKE_BINARY_DIR}/SDDS3-PACKAGE ${OUTPUT}/SDDS3-PACKAGE)
    execute_process(COMMAND cp -a ${DIST}/base/SDDS-Import.xml ${OUTPUT}/SDDS3-PACKAGE/)
    execute_process(COMMAND rm -rf ${OUTPUT}/RA-SDDS-COMPONENT)
    execute_process(COMMAND rm -rf ${OUTPUT}/RA-SDDS3-COMPONENT)
    execute_process(COMMAND cp -a ${DIST}/ra ${OUTPUT}/RA-SDDS-COMPONENT)
    execute_process(COMMAND cp -a ${CMAKE_BINARY_DIR}/RA-SDDS3-PACKAGE ${OUTPUT}/RA-SDDS3-PACKAGE)
    execute_process(COMMAND cp -a ${DIST}/ra/SDDS-Import.xml ${OUTPUT}/RA-SDDS3-PACKAGE/)
    execute_process(COMMAND cp -a ${CMAKE_BINARY_DIR}/products/PluginApi/pluginapi.tar.gz ${OUTPUT}/pluginapi.tar.gz)
    execute_process(COMMAND tar -zcvf ${OUTPUT}/SystemProductTestOutput.tar.gz -C ${CMAKE_BINARY_DIR} SystemProductTestOutput/)
    execute_process(COMMAND bash -c \"[[ -d ${CMAKE_BINARY_DIR}/symbols ]] && cp -a ${CMAKE_BINARY_DIR}/symbols ${OUTPUT}/ && echo copied symbols to output \")
    execute_process(COMMAND cp -a ${CMAKE_BINARY_DIR}/products/ThininstallerDependencies/cmcsrouterapi.tar.gz ${OUTPUT}/cmcsrouterapi.tar.gz)
")
