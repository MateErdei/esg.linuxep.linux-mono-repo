*** Settings ***
Documentation    Test that wdctl can install a plugin configuration

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

*** Test Cases ***
Test that wdctl can install a plugin configuration
    [Tags]    WDCTL
    Require Fresh Install
    Create File    ${TEMPDIR}/test_plugin.json    content={}
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl  copyPluginRegistration    ${TEMPDIR}/test_plugin.json
    Should Be Equal As Integers    ${result.rc}    0
    Should Exist   ${SOPHOS_INSTALL}/base/pluginRegistry/test_plugin.json
