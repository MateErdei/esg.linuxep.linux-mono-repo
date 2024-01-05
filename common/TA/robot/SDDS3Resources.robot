*** Settings ***
Library    OperatingSystem
Library    Process
Library    ${COMMON_TEST_LIBS}/FakeSDDS3WarehouseUtils.py

*** Variables ***
${SDDS_IMPORT_AND_MANIFEST}  ${BASE_SDDS_SCRIPTS}/generateSDDSImportAndManifestDat.py
${SDDS_IMPORT}  ${BASE_SDDS_SCRIPTS}/generateSDDSImport.py
${SDDS3_LOCAL_BASE}   /tmp/sdds3LocalBase
${SDDS3_FAKEWAREHOUSE_DIR}   /tmp/sdds3FakeWarehouse
${SDDS3_FAKEBASE}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakebase
${SDDS3_FAKEPLUGIN}    ${SDDS3_FAKEWAREHOUSE_DIR}/fakeplugin
${SDDS3_FAKESSPLFLAG}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakessplflags
${SDDS3_FAKEREPO}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakerepo
${SDDS3_FAKEPACKAGES}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakerepo/package
${SDDS3_FAKESUITES}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakerepo/suite
${SDDS3_FAKESUPPLEMENT}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakerepo/supplement
${SDDS3_FAKEFLAGS}   ${SDDS3_FAKEWAREHOUSE_DIR}/fakeflag
${nonce}  020fb0c370

*** Keywords ***
Generate Fake sdds3 warehouse
    Create Directories For Fake SDDS3 Warehouse
    ${base_package}=  Generate Fake Base SDDS3 Package

    ${sdds3_base_package_ref} =    Create Package Ref For Sdds3 Suite Xml
    ...    ${base_package}
    ...    SPL-Base-Component
    ...    ServerProtectionLinux-Base-component
    ...    has_flags_supplement=${True}
    Generate Suite dat File    ${sdds3_base_package_ref}

    Generate Fake Supplement  "{\"random\":\"stuff\"\}"
    #launch darkly flag
    write_sdds3_flag

Generate Fake Sdds3 Warehouse With Real Base But Fake Failing Plugin
    Create Directories For Fake SDDS3 Warehouse
    ${plugin_package} =  Generate Fake Plugin SDDS3 Package    installs_successfully=${False}

    ${sdds3_base_package_ref} =    Create Package Ref For Sdds3 Suite Xml
    ...    ${sdds3_base_package}
    ...    SPL-Base-Component
    ...    ServerProtectionLinux-Base-component
    ...    has_flags_supplement=${True}
    ${sdds3_plugin_package_ref} =    Create Package Ref For Sdds3 Suite Xml
    ...    ${plugin_package}
    ...    SPL-Anti-Virus-Plugin
    ...    ServerProtectionLinux-Plugin-AV
    Generate Suite dat File    ${sdds3_base_package_ref}    ${sdds3_plugin_package_ref}
    Copy File  ${sdds3_base_package}  ${SDDS3_FAKEPACKAGES}/

    Generate Fake Supplement  "{\"random\":\"stuff\"\}"
    Write Sdds3 Flag

Generate Fake Sdds3 Warehouse With Fake Failing Base
    Create Directories For Fake SDDS3 Warehouse
    ${base_package} =  Generate Fake Base SDDS3 Package    installs_successfully=${False}

    ${sdds3_base_package_ref} =    Create Package Ref For Sdds3 Suite Xml
    ...    ${base_package}
    ...    SPL-Base-Component
    ...    ServerProtectionLinux-Base-component
    ...    has_flags_supplement=${True}
    Generate Suite dat File    ${sdds3_base_package_ref}

    Generate Fake Supplement  "{\"random\":\"stuff\"\}"
    Write Sdds3 Flag

Generate Warehouse From Local Base Input
    [Arguments]  ${flagscontent}="{\"random\":\"stuff\"\}"
    Create Directories For Fake SDDS3 Warehouse

    ${sdds3_base_package_ref} =    Create Package Ref For Sdds3 Suite Xml
    ...    ${sdds3_base_package}
    ...    SPL-Base-Component
    ...    ServerProtectionLinux-Base-component
    ...    has_flags_supplement=${True}
    Generate Suite dat File    ${sdds3_base_package_ref}
    Copy File  ${sdds3_base_package}  ${SDDS3_FAKEPACKAGES}/

    Generate Fake Supplement  ${flagscontent}
    #launch darkly flag
    write_sdds3_flag


Generate Fake Base SDDS3 Package
    [Arguments]    ${installs_successfully}=${True}
    Create Directory  ${SDDS3_FAKEBASE}
    Create Directory  ${SDDS3_FAKEBASE}/files
    Create Directory  ${SDDS3_FAKEBASE}/files/bin

    ${base_package} =  Set Variable   ${SDDS3_FAKEPACKAGES}/SPL-Base-Component_1.0.0.0.${nonce}.zip

    IF    ${installs_successfully}
        Create File    ${SDDS3_FAKEBASE}/install.sh    \#!/bin/bash\necho \"THIS IS GOOD\" ;\nmkdir ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository/supplement\nexit 0
    ELSE
        Create File    ${SDDS3_FAKEBASE}/install.sh    \#!/bin/bash\necho \"THIS IS BAD\"\nexit 1
    END

    Create File       ${SDDS3_FAKEBASE}/VERSION.ini   PRODUCT_VERSION = 99.99.99
    Create File       ${SDDS3_FAKEBASE}/files/bin/uninstall.sh   \#!/bin/bash\nrm -rf ${SOPHOS_INSTALL}\nexit 0
    Run Process  chmod  +x  ${SDDS3_FAKEBASE}/install.sh
    Run Process  chmod  +x  ${SDDS3_FAKEBASE}/files/bin/uninstall.sh

    #generate sdds import for fake installset
    Set Environment Variable    PRODUCT_NAME   SPL-Base-Component
    Set Environment Variable    PRODUCT_LINE_ID    ServerProtectionLinux-Base-component
    Set Environment Variable    FEATURE_LIST    CORE
    Set Environment Variable    VERSION_OVERRIDE  1.0.0.0
    ${result} =   Run Process     python3 ${SDDS_IMPORT_AND_MANIFEST} ${SDDS3_FAKEBASE}  shell=true
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings   ${result.rc}  0

    #generate package zip file
    ${result1} =   Run Process   ${SDDS3_Builder} --build-package --package-dir ${SDDS3_FAKEPACKAGES} --sdds-import ${SDDS3_FAKEBASE}/SDDS-Import.xml --nonce ${nonce}  shell=true
    Log  ${result1.stdout}
    Log  ${result1.stderr}
    Should Be Equal As Strings   ${result1.rc}  0
    File Should exist  ${base_package}

    [Return]  ${base_package}

Generate Fake Plugin SDDS3 Package
    [Arguments]    ${installs_successfully}=${True}
    Create Directory  ${SDDS3_FAKEPLUGIN}

    ${package} =  Set Variable   ${SDDS3_FAKEPACKAGES}/SPL-Anti-Virus-Plugin_1.0.0.0.${nonce}.zip

    IF    ${installs_successfully}
        Create File    ${SDDS3_FAKEPLUGIN}/install.sh    \#!/bin/bash\necho \"THIS IS GOOD\"\nexit 0
    ELSE
        Create File    ${SDDS3_FAKEPLUGIN}/install.sh    \#!/bin/bash\necho \"THIS IS BAD\"\nexit 1
    END

    Create File    ${SDDS3_FAKEPLUGIN}/VERSION.ini    PRODUCT_VERSION = 99.99.99
    Run Process    chmod  +x  ${SDDS3_FAKEPLUGIN}/install.sh

    #generate sdds import for fake installset
    Set Environment Variable    PRODUCT_NAME    SPL-Anti-Virus-Plugin
    Set Environment Variable    PRODUCT_LINE_ID    ServerProtectionLinux-Plugin-AV
    Set Environment Variable    FEATURE_LIST    CORE
    Set Environment Variable    VERSION_OVERRIDE    1.0.0.0
    ${result} =   Run Process     python3 ${SDDS_IMPORT_AND_MANIFEST} ${SDDS3_FAKEPLUGIN}  shell=true
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings   ${result.rc}  0

    #generate package zip file
    ${result1} =   Run Process   ${SDDS3_Builder} --build-package --package-dir ${SDDS3_FAKEPACKAGES} --sdds-import ${SDDS3_FAKEPLUGIN}/SDDS-Import.xml --nonce ${nonce}  shell=true
    Log  ${result1.stdout}
    Log  ${result1.stderr}
    Should Be Equal As Strings   ${result1.rc}  0
    File Should exist  ${package}

    [Return]  ${package}

Generate Local Base SDDS3 Package
    ${result} =   Get Folder With Installer
    #generate package zip file

    ${result1} =   Run Process   ${SDDS3_Builder} --build-package --package-dir ${SDDS3_LOCAL_BASE} --sdds-import ${result}/SDDS-Import.xml --nonce ${nonce}  shell=true    stdout=/tmp/sdds3.log    stderr=STDOUT
    Log File    /tmp/sdds3.log
    Remove File    /tmp/sdds3.log

    Should Be Equal As Strings   ${result1.rc}  0
    ${Files} =  List Files In Directory   ${SDDS3_LOCAL_BASE}  *.zip
    ${base_package} =  Set Variable   ${SDDS3_LOCAL_BASE}/${Files}[0]
    File Should exist  ${base_package}
    Log Hash of File  ${base_package}

    [Return]    ${base_package}

Clean up local Base SDDS3 Package
    Remove Directory  ${SDDS3_LOCAL_BASE}  recursive=True

Generate Suite dat File
    [Arguments]    @{package_refs}
    #generate suite .dat file
    Write Sdds3 Suite Xml    ${package_refs}
    ${result1} =   Run Process
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
    ...  python3  ${SDDS_IMPORT}  ${SDDS3_FAKESSPLFLAG}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers   ${result.rc}  ${0}

    ${result1} =   Run Process
    ...  ${SDDS3_Builder}  --build-package  --package-dir  ${SDDS3_FAKEPACKAGES}
    ...  --sdds-import  ${SDDS3_FAKESSPLFLAG}/SDDS-Import.xml
    ...  --nonce  ${nonce}
    Log  ${result1.stdout}
    Log  ${result1.stderr}
    Should Be Equal As Integers   ${result1.rc}  ${0}

    #generate supplment dat file
    write_sdds3_supplement_xml  ${SDDS3_FAKEPACKAGES}/SSPLFLAGS_1.0.0.020fb0c370.zip
    ${result1} =   Run Process   ${SDDS3_Builder} --build-supplement --xml ${SDDS3_FAKEREPO}/supplement.xml --dir ${SDDS3_FAKESUPPLEMENT} --nonce ${nonce}  shell=true
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
    [Arguments]  ${port}=8080  ${cert}=${COMMON_TEST_UTILS}/server_certs/server.crt
    ${handle}=  Start Process  python3 ${COMMON_TEST_LIBS}/SDDS3server.py --launchdarkly ${SDDS3_FAKEFLAGS} --sdds3 ${SDDS3_FAKEREPO} --port ${port} --cert ${cert}  shell=true
    [Return]  ${handle}
