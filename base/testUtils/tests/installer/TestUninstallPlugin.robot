*** Settings ***
Documentation    Test base uninstaller calls plugin uninstaller

Library    ${libs_directory}/FullInstallerUtils.py

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

*** Variables ***

${tmpdir}                       ${SOPHOS_INSTALL}/tmp
${InstallProductsDir}           ${SOPHOS_INSTALL}/base/update/var/installedproducts

*** Test Cases ***
Test base uninstaller calls plugin uninstaller
    [Tags]    UNINSTALL
    Require Fresh Install

    ## Temporary - until installer creates this directory
    Create Directory  ${InstallProductsDir}

    Remove File  /tmp/TEST_CASE_PLUGIN_UNINSTALLER_RUN

    Create Uninstall File  0  "PLUGIN_UNINSTALLER_CALLED"  TEST_CASE_PLUGIN   /tmp/TEST_CASE_PLUGIN_UNINSTALLER_RUN
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/uninstall.sh  --force
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Should Be Equal As Integers    ${result.rc}    0        uninstaller failed with ${result.rc}
    Should Exist   /tmp/TEST_CASE_PLUGIN_UNINSTALLER_RUN


*** Keywords ***
Create Uninstall File
    [Arguments]    ${Exitcode}   ${Message}   ${ProductName}  ${dest}
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...    echo '${Message}'
    ...    echo '${Message}' >${dest}
    ...    exit ${ExitCode}
    ...    \
    Create File   ${tmpdir}/${ProductName}.sh    content=${script}
    ${result} =   Run Process   ln  -s  ${tmpdir}/${ProductName}.sh  ${InstallProductsDir}/${ProductName}.sh
    Should Be Equal As Integers    ${result.rc}    0        Failed to create symlink: ln -s ${tmpdir}/${ProductName}.sh ${InstallProductsDir}/${ProductName}.sh
