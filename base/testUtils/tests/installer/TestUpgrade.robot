*** Settings ***
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Resource  InstallerResources.robot
Resource  ../GeneralTeardownResource.robot

Test Teardown  Upgrade Test Teardown
Default Tags  INSTALLER  TAP_TESTS

*** Test Cases ***
Simple Upgrade Test
    Require Fresh Install
    ${result} =   Get Folder With Installer
    ${BaseDevVersion} =     Get Version Number From Ini File   ${SOPHOS_INSTALL}/base/VERSION.ini
    Copy Directory  ${result}  /opt/tmp/version2
    Replace Version  ${BaseDevVersion}   9.99.999  /opt/tmp/version2
    ${result} =  Run Process  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0
    ${BaseDevVersion2} =     Get Version Number From Ini File   ${SOPHOS_INSTALL}/base/VERSION.ini
    Should Not Be Equal As Strings  ${BaseDevVersion}  ${BaseDevVersion2}

    Check Expected Base Processes Are Running

    Mark Expected Critical In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.mcs <> Not registered: MCSID is not present
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

*** Keywords ***
Upgrade Test Teardown
    General Test Teardown
    Remove Directory  /opt/tmp/version2  true
    Require Uninstalled
