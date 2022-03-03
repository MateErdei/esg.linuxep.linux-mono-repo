set(DISTRIBUTION_TOOL_DIR ${CMAKE_SOURCE_DIR}/products/distribution)
set(STRIP_EXECUTABLES ${DISTRIBUTION_TOOL_DIR}/stripExecutables.py)
set(ADD_SYMBOLS_SCRIPT ${DISTRIBUTION_TOOL_DIR}/addSymbols.sh)
set(SYMBOLS_DIR ${CMAKE_BINARY_DIR}/symbols)
set(GENERATE_SDDS_IMPORT_MANIFEST_DAT ${DISTRIBUTION_TOOL_DIR}/generateSDDSImportAndManifestDat.py)

INSTALL(CODE "MESSAGE(STATUS \"Post install steps\")")

# Strip executables
INSTALL(CODE "MESSAGE(STATUS \"Stripping executables\")
    execute_process(COMMAND python3 ${STRIP_EXECUTABLES} ${DIST} ${SYMBOLS_DIR} ${CMAKE_BUILD_TYPE})")

# Copy symbols to output
INSTALL(CODE "MESSAGE(STATUS \"Copying symbols to output\")
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${ADD_SYMBOLS_SCRIPT} ${SYMBOLS_DIR}/)")

# Generate SDDS2 manifests
INSTALL(CODE "MESSAGE(STATUS \"Generating SDDS2 manifests\")
    set(ENV{PRODUCT_NAME} \"${PRODUCT_NAME}\")
    set(ENV{PRODUCT_LINE_ID} \"${PRODUCT_LINE_ID}\")
    set(ENV{DEFAULT_HOME_FOLDER} \"${DEFAULT_HOME_FOLDER}\")
    set(ENV{FEATURE_LIST} \"${FEATURE_LIST}\")
    execute_process(COMMAND python3 ${GENERATE_SDDS_IMPORT_MANIFEST_DAT} ${DIST} ${CMAKE_BUILD_TYPE} ${CMAKE_SOURCE_DIR})")

# Generate SDDS3 manifests
INSTALL(CODE "MESSAGE(STATUS \"Generating SDDS3 manifests\")
    execute_process(COMMAND ${INPUT}/sdds3/sdds3-builder --build-package --package-dir ${CMAKE_BINARY_DIR}/SDDS3-PACKAGE --sdds-import ${DIST}/SDDS-Import.xml --nonce `sha256sum ${DIST}/SDDS-Import.xml | head -c 10`)")

if (NOT EXISTS ${INPUT}/sdds3/sdds3-builder)
    message(FATAL_ERROR "Could not find SDDS3 Builder: ${INPUT}/sdds3/sdds3-builder")
endif()
