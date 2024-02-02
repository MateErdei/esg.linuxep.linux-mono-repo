*** Settings ***
Documentation    Test that wdctl can install a plugin configuration

Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py

Resource   ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource   ${COMMON_TEST_ROBOT}/InstallerResources.robot

Force Tags  TAP_PARALLEL5

*** Test Cases ***
Test that wdctl can install a plugin configuration
    [Tags]    WDCTL
    Require Fresh Install
    Create File    ${SOPHOS_INSTALL}/tmp/test_plugin.json    content={}
    ${result} =    Run Process    ${SOPHOS_INSTALL}/bin/wdctl  copyPluginRegistration    ${SOPHOS_INSTALL}/tmp/test_plugin.json
    Should Be Equal As Integers    ${result.rc}    0
    Should Exist   ${SOPHOS_INSTALL}/base/pluginRegistry/test_plugin.json
