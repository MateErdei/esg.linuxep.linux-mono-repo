*** Settings ***
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Resource  InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../upgrade_product/UpgradeResources.robot

Test Teardown  Upgrade Test Teardown
Default Tags  INSTALLER  TAP_TESTS

*** Test Cases ***
Simple Upgrade Test
    Require Fresh Install
    ${result} =   Get Folder With Installer
    ${BaseDevVersion} =     Get Version Number From Ini File   ${SOPHOS_INSTALL}/base/VERSION.ini
    Copy Directory  ${result}  /opt/tmp/version2
    Replace Version  ${BaseDevVersion}   9.99.999  /opt/tmp/version2
    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0
    Create File  ${SOPHOS_INSTALL}/base/mcs/action/testfile
    Should Exist  ${SOPHOS_INSTALL}/base/mcs/action/testfile
    ${result} =  Run Process  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0
    Should Not exist  ${SOPHOS_INSTALL}/base/mcs/action/testfile
    ${BaseDevVersion2} =     Get Version Number From Ini File   ${SOPHOS_INSTALL}/base/VERSION.ini
    Should Not Be Equal As Strings  ${BaseDevVersion}  ${BaseDevVersion2}

    Check Expected Base Processes Are Running

    Mark Expected Critical In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.mcs <> Not registered: MCSID is not present
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Simple Downgrade Test
    Require Fresh Install
    ${distribution} =   Get Folder With Installer
    Create File  ${SOPHOS_INSTALL}/bin/blah
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy
    Directory Should Not Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup
    ${result} =  Run Process   ${SOPHOS_INSTALL}/bin/uninstall.sh  --downgrade  --force

    Should Be Equal As Integers    ${result.rc}    0
    Directory Should Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup
    Should not exist  ${SOPHOS_INSTALL}/bin/blah
    Should not exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy

    ${BaseDevVersion} =     Get Version Number From Ini File   ${SOPHOS_INSTALL}/base/VERSION.ini
    Copy Directory  ${distribution}  /opt/tmp/version2
    Replace Version  ${BaseDevVersion}   0.1.1  /opt/tmp/version2

    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0

    ${result} =  Run Process  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0

    ${BaseDevVersion2} =     Get Version Number From Ini File   ${SOPHOS_INSTALL}/base/VERSION.ini
    Should Not Be Equal As Strings  ${BaseDevVersion}  ${BaseDevVersion2}

    Check Expected Base Processes Are Running

    Mark Expected Critical In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.mcs <> Not registered: MCSID is not present
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Check Local Logger Config Settings Are Processed and Persist After Upgrade
    ${expected_local_file_contents} =  Set Variable  [global]\nVERBOSITY = DEBUG\n

    Require Fresh Install
    Check Expected Base Processes Are Running

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Watchdog Log Contains  configured for level: INFO

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check mcsrouter Log Contains  Logging level: 20

    Override Local LogConf File for a component   DEBUG  global
    Restart And Remove MCS And Watchdog Logs

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Watchdog Log Contains  configured for level: DEBUG

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check mcsrouter Log Contains  Logging level: 10

    Run Full Installer

    ${actual_local_file_contents} =  Get File  ${SOPHOS_INSTALL}/base/etc/logger.conf.local
    Should Be Equal As Strings  ${expected_local_file_contents}  ${actual_local_file_contents}


*** Keywords ***

Restart And Remove MCS And Watchdog Logs
    Run Shell Process  systemctl stop sophos-spl    OnError=failed to restart sophos-spl
    Remove File  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Run Shell Process  systemctl start sophos-spl   OnError=failed to restart sophos-spl

Upgrade Test Teardown
    LogUtils.Dump Log  ${SOPHOS_INSTALL}/base/etc/logger.conf
    LogUtils.Dump Log  ${SOPHOS_INSTALL}/base/etc/logger.conf.local
    General Test Teardown
    Remove Directory  /opt/tmp/version2  true
    Require Uninstalled
