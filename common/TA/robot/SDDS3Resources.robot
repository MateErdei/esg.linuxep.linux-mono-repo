*** Settings ***
Library    OperatingSystem
Library    Process
Library    ${COMMON_TEST_LIBS}/FakeSDDS3WarehouseUtils.py

*** Variables ***
${SDDS_IMPORT_AND_MANIFEST}  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/generateSDDSImportAndManifestDat.py
${SDDS_IMPORT}  ${SYSTEM_PRODUCT_TEST_OUTPUT_PATH}/generateSDDSImport.py
${SDDS3_LOCAL_BASE}   /tmp/sdds3LocalBase
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
    Generate Fake Supplement  "{\"random\":\"stuff\"\}"
    #launch darkly flag
    write_sdds3_flag

Generate Warehouse From Local Base Input
    [Arguments]  ${flagscontent}="{\"random\":\"stuff\"\}"
    Create Directories For Fake SDDS3 Warehouse

    Generate Suite dat File  ${sdds3_base_package}
    Copy File  ${sdds3_base_package}  ${SDDS3_FAKEPACKAGES}/

    Generate Fake Supplement  ${flagscontent}
    #launch darkly flag
    write_sdds3_flag
    Create Directory   ${SDDS3_DEVCERTS}
    Copy File   ${SUPPORT_FILES}/sophos_certs/rootca384.crt    ${SDDS3_DEVCERTS}/


Generate Fake Base SDDS3 Package
    Create Directory  ${SDDS3_FAKEBASE}
    Create Directory  ${SDDS3_FAKEBASE}/files
    Create Directory  ${SDDS3_FAKEBASE}/files/bin

    ${base_package} =  Set Variable   ${SDDS3_FAKEPACKAGES}/SPL-Base-Component_1.0.0.0.${nonce}.zip

    Create File       ${SDDS3_FAKEBASE}/install.sh   \#!/bin/bash\necho \"THIS IS GOOD\" ;\nmkdir ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository/supplement\nexit 0
    Create File       ${SDDS3_FAKEBASE}/VERSION.ini   PRODUCT_VERSION = 99.99.99
    Create File       ${SDDS3_FAKEBASE}/files/bin/h   \#!/bin/bash\nrm -rf ${SOPHOS_INSTALL}\nexit 0
    Run Process  chmod  +x  ${SDDS3_FAKEBASE}/install.sh
    Run Process  chmod  +x  ${SDDS3_FAKEBASE}/files/bin/uninstall.sh

    #generate sdds import for fake installset
    Set Environment Variable    PRODUCT_NAME   SPL-Base-Component
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

Generate Local Base SDDS3 Package
    ${result} =   Get Folder With Installer
    #generate package zip file

    ${result1} =   Run Process   bash ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh ${SDDS3_Builder} --build-package --package-dir ${SDDS3_LOCAL_BASE} --sdds-import ${result}/SDDS-Import.xml --nonce ${nonce}  shell=true    stdout=/tmp/sdds3.log    stderr=STDOUT
    Log File    /tmp/sdds3.log
    Remove File    /tmp/sdds3.log

    Should Be Equal As Strings   ${result1.rc}  0
    ${Files} =  List Files In Directory   ${SDDS3_LOCAL_BASE}  *.zip
    ${base_package} =  Set Variable   ${SDDS3_LOCAL_BASE}/${Files}[0]
    File Should exist  ${base_package}
    [Return]    ${base_package}

Clean up local Base SDDS3 Package
    Remove Directory  ${SDDS3_LOCAL_BASE}  recursive=True

Generate Suite dat File
    [Arguments]  ${package}
    #generate suite .dat file
    write_sdds3_suite_xml  ${package}
    ${result1} =   Run Process
    ...  bash  -x  ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh
    ...  ${SDDS3_Builder}  --build-suite  --xml  ${SDDS3_FAKEREPO}/suite.xml
    ...  --dir  ${SDDS3_FAKESUITES}  --nonce  ${nonce}
    Log  ${result1.stdout}
    Log  ${result1.stderr}
    Should Be Equal As Integers   ${result1.rc}  ${0}

Generate Fake Supplement
    [Arguments]  ${flagscontent}
    Create Directory  ${SDDS3_FAKESSPLFLAG}

    Create File       ${SDDS3_FAKESSPLFLAG}/flags-warehouse.json   ${flagscontent}

    Set Environment Variable    PRODUCT_NAME   SSPLFLAGS
    Set Environment Variable    FEATURE_LIST    CORE
    Set Environment Variable    VERSION_OVERRIDE  1.0.0
    ${result} =   Run Process
    ...  bash  -x  ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh
    ...  python3  ${SDDS_IMPORT}  ${SDDS3_FAKESSPLFLAG}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers   ${result.rc}  ${0}

    ${result1} =   Run Process
    ...  bash  -x  ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh
    ...  ${SDDS3_Builder}  --build-package  --package-dir  ${SDDS3_FAKEPACKAGES}
    ...  --sdds-import  ${SDDS3_FAKESSPLFLAG}/SDDS-Import.xml
    ...  --nonce  ${nonce}
    Log  ${result1.stdout}
    Log  ${result1.stderr}
    Should Be Equal As Integers   ${result1.rc}  ${0}

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
    ${handle}=  Start Process  bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${COMMON_TEST_LIBS}/SDDS3server.py --launchdarkly ${SDDS3_FAKEFLAGS} --sdds3 ${SDDS3_FAKEREPO} --port ${port} --cert ${cert}  shell=true
    [Return]  ${handle}
