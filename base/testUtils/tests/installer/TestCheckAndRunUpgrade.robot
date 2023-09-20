*** Settings ***
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../upgrade_product/UpgradeResources.robot
Resource  ../ra_plugin/ResponseActionsResources.robot

Suite Setup    Require Fresh Install
Suite Teardown    Require Uninstalled
Test Teardown  Upgrade Test Teardown

Default Tags  INSTALLER  TAP_PARALLEL2  RA_PLUGIN
*** Test Cases ***
Test TestCheckAndRunUpgrade with huge version.ini
    Require Fresh Install
    Install Response Actions Directly

    ${RESPONSE_ACTIONS_SDDS_DIR} =  Get SSPL Response Actions Plugin SDDS
    Copy Directory  ${RESPONSE_ACTIONS_SDDS_DIR}  /opt/tmp/version2
    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0


    ${RADevVersion} =     Get Version Number From Ini File   ${SOPHOS_INSTALL}/plugins/responseactions/VERSION.ini
    replace_string_in_file  ${RADevVersion}   9.990000000000000000000000000000000000000000000000000000000000000000000.99000000000000000000000000000000000000000000000000000000000000000000000009  ${SOPHOS_INSTALL}/plugins/responseactions/VERSION.ini

    Create File    /opt/tmp/version2/upgrade/9.990000000000000000000000000000000000000000000000000000000000000000000.sh     \#!/bin/bash\ntouch /opt/sophos-spl/plugins/responseactions/stuff\nexit 0
    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/upgrade/9.990000000000000000000000000000000000000000000000000000000000000000000.sh
    Should Be Equal As Integers    ${result.rc}    0


    ${result} =  Run Process  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0
    Log     ${result.stdout}
    Log     ${result.stderr}

    Should Exist  /opt/sophos-spl/plugins/responseactions/stuff
    Check Response Actions Executable Running

    Mark Expected Critical In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.mcs <> Not registered: MCSID is not present
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with exit code 1
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Test TestCheckAndRunUpgrade with malformed version.ini
    Require Fresh Install
    Install Response Actions Directly

    ${RESPONSE_ACTIONS_SDDS_DIR} =  Get SSPL Response Actions Plugin SDDS
    Copy Directory  ${RESPONSE_ACTIONS_SDDS_DIR}  /opt/tmp/version2
    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0


    ${RADevVersion} =     Get Version Number From Ini File   ${SOPHOS_INSTALL}/plugins/responseactions/VERSION.ini
    replace_string_in_file  ${RADevVersion}   9  ${SOPHOS_INSTALL}/plugins/responseactions/VERSION.ini

    Create File    /opt/tmp/version2/upgrade/9.sh     \#!/bin/bash\ntouch /opt/sophos-spl/plugins/responseactions/stuff\nexit 0
    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/upgrade/9.sh
    Should Be Equal As Integers    ${result.rc}    0


    ${result} =  Run Process  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0
    Log     ${result.stdout}
    Log     ${result.stderr}

    Should Exist  /opt/sophos-spl/plugins/responseactions/stuff
    Check Response Actions Executable Running

    Mark Expected Critical In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.mcs <> Not registered: MCSID is not present
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with exit code 1
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Test TestCheckAndRunUpgrade with ignores upgrade scripts with names that do not match version in version.ini
    Require Fresh Install
    Install Response Actions Directly

    ${RESPONSE_ACTIONS_SDDS_DIR} =  Get SSPL Response Actions Plugin SDDS
    Copy Directory  ${RESPONSE_ACTIONS_SDDS_DIR}  /opt/tmp/version2
    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0


    Create File    /opt/tmp/version2/upgrade/6.7.9.sh     \#!/bin/bash\ntouch /opt/sophos-spl/plugins/responseactions/stuff\nexit 0
    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/upgrade/6.7.9.sh
    Should Be Equal As Integers    ${result.rc}    0
    Create File    /opt/tmp/version2/upgrade/upgrade.sh     \#!/bin/bash\ntouch /opt/sophos-spl/plugins/responseactions/stuff\nexit 0
    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/upgrade/upgrade.sh
    Should Be Equal As Integers    ${result.rc}    0
    Create File    /opt/tmp/version2/upgrade/.sh     \#!/bin/bash\ntouch /opt/sophos-spl/plugins/responseactions/stuff\nexit 0
    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/upgrade/.sh
    Should Be Equal As Integers    ${result.rc}    0

    ${result} =  Run Process  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0
    Log     ${result.stdout}
    Log     ${result.stderr}

    Should Not Exist  /opt/sophos-spl/plugins/responseactions/stuff
    Check Response Actions Executable Running

    Mark Expected Critical In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.mcs <> Not registered: MCSID is not present
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with exit code 1
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Test TestCheckAndRunUpgrade with logs to stdout if version.ini does not have a version
    Require Fresh Install
    Install Response Actions Directly

    ${RESPONSE_ACTIONS_SDDS_DIR} =  Get SSPL Response Actions Plugin SDDS
    Copy Directory  ${RESPONSE_ACTIONS_SDDS_DIR}  /opt/tmp/version2
    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0
    replace_string_in_file  PRODUCT_VERSION   STUFF  ${SOPHOS_INSTALL}/plugins/responseactions/VERSION.ini
    log File  ${SOPHOS_INSTALL}/plugins/responseactions/VERSION.ini

    ${result} =  Run Process  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0
    Log     ${result.stdout}
    Log     ${result.stderr}
    Should Contain     ${result.stdout}     /opt/sophos-spl/plugins/responseactions/VERSION.ini is malformed it does not contain a product version
    Should Not Exist  /opt/sophos-spl/plugins/responseactions/stuff
    Check Response Actions Executable Running

    Mark Expected Critical In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.mcs <> Not registered: MCSID is not present
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with exit code 1
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical
*** Keywords ***
Upgrade Test Teardown
    General Test Teardown
    Remove Directory  /opt/tmp/version2  true
    Uninstall Response Actions