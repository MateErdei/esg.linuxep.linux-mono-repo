*** Settings ***
Documentation    Suite description
Library     ${LIBS_DIRECTORY}/LogUtils.py


Resource  ../upgrade_product/UpgradeResources.robot
Resource  ../installer/InstallerResources.robot
Resource  MDRResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource  ../GeneralTeardownResource.robot
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py

Test Setup  Test Setup
Test Teardown  Test Teardown

Suite Setup   Setup Ostia Warehouse Environment
Suite Teardown   Teardown Ostia Warehouse Environment

Default Tags   MDR_PLUGIN   MANAGEMENT_AGENT  OSTIA  WAREHOUSE_SYNC

*** Variables ***
${MDR_PLUGIN_PATH}  ${SOPHOS_INSTALL}/plugins/mtr/
${MDR_VUT_POLICY}   ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_mtr_VUT.xml


*** Test Cases ***

Test MDR Plugin Can Be Installed Removed and ReInstalled From Central
    [Documentation]  Address bug found LINUXEP-8233
    [Timeout]  10 minutes
    Create File  ${UPDATE_CONFIG}
    Block Connection Between EndPoint And FleetManager

    Remove File  /opt/sophos-spl/base/mcs/status/ALC_status.xml
    Remove File  /opt/sophos-spl/base/mcs/status/cache/ALC.xml

    Simulate Send Policy And Simulate Update Now  ${MDR_VUT_POLICY}

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  3 secs
    ...  File Should Exist  /opt/sophos-spl/base/mcs/status/ALC_status.xml

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check MDR Installed

    Set Log Level For Component And Reset and Return Previous Log  suldownloader  DEBUG

    Remove File  /opt/sophos-spl/base/mcs/status/ALC_status.xml
    Remove File  /opt/sophos-spl/base/mcs/status/cache/ALC.xml

    Simulate Send Policy And Simulate Update Now   ${MDR_VUT_POLICY}   remove_features=MDR   remove_subscriptions=MDR

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  3 secs
    ...  File Should Exist  /opt/sophos-spl/base/mcs/status/ALC_status.xml

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check MDR Uninstalled

    Remove File  /opt/sophos-spl/base/mcs/status/ALC_status.xml
    Remove File  /opt/sophos-spl/base/mcs/status/cache/ALC.xml

    Simulate Send Policy And Simulate Update Now  ${MDR_VUT_POLICY}

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  3 secs
    ...  File Should Exist  /opt/sophos-spl/base/mcs/status/ALC_status.xml

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   Check MDR Installed


Test MDR Plugin When Installed Emit Status With The Components Of The Warehouse Installed
    Create File  ${UPDATE_CONFIG}
    Block Connection Between EndPoint And FleetManager
    Simulate Send Policy And Run Update And Check Success  ${MDR_VUT_POLICY}
    Check MDR Installed
    Check Components Inside The ALC Status

*** Keywords ***
Check Components Inside The ALC Status
    ${statusContent} =  Get File  ${statusPath}
    Should Contain  ${statusContent}  ServerProtectionLinux-MDR-DBOS-Component
    Should Contain  ${statusContent}  ServerProtectionLinux-MDR-Control-Component
    Should Contain  ${statusContent}  ServerProtectionLinux-Base
    Should Contain  ${statusContent}  ServerProtectionLinux-MDR-osquery-Component

Test Teardown
    General Test Teardown
    Restore Connection Between EndPoint and FleetManager
    Teardown Servers For Update Scheduler

Test Setup
    Setup Update Scheduler Environment
    Setup Environment After Warehouse Generation

Check MDR Uninstalled
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr
