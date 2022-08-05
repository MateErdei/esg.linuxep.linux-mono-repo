*** Settings ***
Library     ${LIBS_DIRECTORY}/FakeSDDS3WarehouseUtils.py

*** Variables ***
${SDDS_IMPORT_AND_MANIFEST}  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/generateSDDSImportAndManifestDat.py
${SDDS_IMPORT}  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/generateSDDSImport.py
${SDDS3_Builder}  ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/sdds3-builder
${SDDS3_FAKEWAREHOUSE_DIR}   /tmp/sdds3FakeWarehouse
${SDDS3_FAKEBASE}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakebase
${SDDS3_FAKESSPLFLAG}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakessplflags
${SDDS3_FAKEREPO}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakerepo
${SDDS3_FAKEPACKAGES}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakerepo/package
${SDDS3_FAKESUITES}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakerepo/suite
${SDDS3_FAKESUPPLEMENT}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakerepo/supplement
${SDDS3_FAKEFLAGS}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakeflag
${SDDS3_DEVCERTS}   ${SDDS3_FAKEWAREHOUSE_DIR}/certs
${nonce}  020fb0c370
*** Keywords ***
Generate Fake sdds3 warehouse
    Create Directories For Fake SDDS3 Warehouse
    ${base_package}=  Generate Fake Base SDDS3 Package

    Generate Suite dat File  ${base_package}
    Generate Fake Supplement
    #launch darkly flag
    write_sdds3_flag

Generate Warehouse From Local Base Input
    Create Directories For Fake SDDS3 Warehouse
    Create Directory   ${SDDS3_FAKEPACKAGES}
    ${Files} =  List Files In Directory  ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base-sdds3  *.zip
    Copy File  ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base-sdds3/${Files[0]}  ${SDDS3_FAKEPACKAGES}/
    Generate Suite dat File  ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base-sdds3/${Files[0]}
    Generate Fake Supplement
    #launch darkly flag
    write_sdds3_flag
    Create Directory   ${SDDS3_DEVCERTS}
    Copy File   ${SUPPORT_FILES}/sophos_certs/rootca384.crt    ${SDDS3_DEVCERTS}/


Generate Fake Base SDDS3 Package
    Create Directory  ${SDDS3_FAKEBASE}
    Create Directory  ${SDDS3_FAKEBASE}/files
    Create Directory  ${SDDS3_FAKEBASE}/files/bin

    ${base_package} =  Set Variable   ${SDDS3_FAKEPACKAGES}/SophosServerProtectionLinux-BaseComponent_1.0.0.0.${nonce}.zip

    Create File       ${SDDS3_FAKEBASE}/install.sh   \#!/bin/bash\necho \"THIS IS GOOD\" ;\nmkdir ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository/supplement\nexit 0
    Create File       ${SDDS3_FAKEBASE}/VERSION.ini   PRODUCT_VERSION = 99.99.99
    Create File       ${SDDS3_FAKEBASE}/files/bin/h   \#!/bin/bash\nrm -rf ${SOPHOS_INSTALL}\nexit 0
    Run Process  chmod  +x  ${SDDS3_FAKEBASE}/install.sh
    Run Process  chmod  +x  ${SDDS3_FAKEBASE}/files/bin/uninstall.sh

    #generate sdds import for fake installset
    Set Environment Variable    PRODUCT_NAME   Sophos Server Protection Linux - Base Component
    Set Environment Variable    FEATURE_LIST    CORE
    Set Environment Variable    VERSION_OVERRIDE  1.0.0.0
    ${result} =   Run Process     bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${SDDS_IMPORT_AND_MANIFEST} ${SDDS3_FAKEBASE}  shell=true
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings   ${result.rc}  0

    #generate package zip file
    ${result1} =   Run Process   bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh ${SDDS3_Builder} --build-package --package-dir ${SDDS3_FAKEPACKAGES} --sdds-import ${SDDS3_FAKEBASE}/SDDS-Import.xml --nonce ${nonce}  shell=true
    Log  ${result1.stdout}
    Log  ${result1.stderr}
    Should Be Equal As Strings   ${result1.rc}  0
    File Should exist  ${base_package}

    [Return]  ${base_package}

Generate Suite dat File
    [Arguments]  ${package}
    #generate suite .dat file
    write_sdds3_suite_xml  ${package}
    ${result1} =   Run Process   bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh ${SDDS3_Builder} --build-suite --xml ${SDDS3_FAKEREPO}/suite.xml --dir ${SDDS3_FAKESUITES} --nonce ${nonce}  shell=true
    Log  ${result1.stdout}
    Log  ${result1.stderr}
    Should Be Equal As Strings   ${result1.rc}  0

Generate Fake Supplement
    Create Directory  ${SDDS3_FAKESSPLFLAG}
    Create Directory  ${SDDS3_FAKESSPLFLAG}/content

    Create File       ${SDDS3_FAKESSPLFLAG}/content/flags-warehouse.json   {}

    Set Environment Variable    PRODUCT_NAME   SSPLFLAGS
    Set Environment Variable    FEATURE_LIST    CORE
    Set Environment Variable    VERSION_OVERRIDE  1.0.0
    ${result} =   Run Process     bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${SDDS_IMPORT} ${SDDS3_FAKESSPLFLAG}  shell=true
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings   ${result.rc}  0

    ${result1} =   Run Process   bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh ${SDDS3_Builder} --build-package --package-dir ${SDDS3_FAKEPACKAGES} --sdds-import ${SDDS3_FAKESSPLFLAG}/SDDS-Import.xml --nonce ${nonce}  shell=true
    Log  ${result1.stdout}
    Log  ${result1.stderr}
    Should Be Equal As Strings   ${result1.rc}  0

    #generate supplment dat file
    write_sdds3_supplement_xml  ${SDDS3_FAKEPACKAGES}/SSPLFLAGS_1.0.0.020fb0c370.zip
    ${result1} =   Run Process   bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh ${SDDS3_Builder} --build-supplement --xml ${SDDS3_FAKEREPO}/supplement.xml --dir ${SDDS3_FAKESUPPLEMENT} --nonce ${nonce}  shell=true
    Log  ${result1.stdout}
    Log  ${result1.stderr}
    Should Be Equal As Strings   ${result1.rc}  0

Create Directories For Fake SDDS3 Warehouse
    Create Directory  ${SDDS3_FAKEWAREHOUSE_DIR}
    Create Directory  ${SDDS3_FAKEREPO}
    Create Directory  ${SDDS3_FAKESUITES}
    Create Directory  ${SDDS3_FAKEFLAGS}
    Create Directory  ${SDDS3_FAKESUPPLEMENT}

Clean up fake warehouse
    Remove Environment Variable  PRODUCT_NAME
    Remove Environment Variable  FEATURE_LIST
    Remove Environment Variable  VERSION_OVERRIDE
    Remove Directory  ${SDDS3_FAKEWAREHOUSE_DIR}  recursive=True

Start Local SDDS3 server with fake files
    [Arguments]  ${port}=8080  ${cert}=${SUPPORT_FILES}/https/server-private.pem
    ${handle}=  Start Process  bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIBS_DIRECTORY}/SDDS3server.py --launchdarkly ${SDDS3_FAKEFLAGS} --sdds3 ${SDDS3_FAKEREPO} --port ${port} --cert ${cert}  shell=true
    [Return]  ${handle}