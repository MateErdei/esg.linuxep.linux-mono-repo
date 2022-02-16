*** Settings ***
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py

Resource    AVResources.robot

Suite Setup     Require Fresh Install
Suite Teardown  Require Uninstalled

Test Teardown  AV Test Teardown

Force Tags  LOAD4
Default Tags   AV_PLUGIN


*** Test Cases ***
AV Plugin Installs With Version Ini File
    Install AV Plugin Directly
    Check AV Plugin Installed Directly
    File Should Exist   ${SOPHOS_INSTALL}/plugins/av/VERSION.ini
    VERSION Ini File Contains Proper Format For Product Name   ${SOPHOS_INSTALL}/plugins/av/VERSION.ini   Sophos Server Protection Linux - av

*** Keywords ***
AV Test Teardown
    Run Keyword If Test Failed  Dump Teardown Log  /tmp/av_install.log
    Remove File  /tmp/av_install.log
    General Test Teardown