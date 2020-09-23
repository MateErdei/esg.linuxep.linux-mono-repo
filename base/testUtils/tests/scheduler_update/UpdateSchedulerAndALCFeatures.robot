*** Settings ***
Documentation    Check UpdateScheduler Plugin using real SulDownloader and the Fake Warehouse to prove it works.

Test Teardown   Teardown Servers For Update Scheduler
Test Setup  Setup Update Scheduler Environment

Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Resource  SchedulerUpdateResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../mdr_plugin/MDRResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot


Default Tags   SULDOWNLOADER  UPDATE_SCHEDULER

*** Variables ***
${MDR_RIGID_NAME}   ServerProtectionLinux-Plugin-MDR
${MDR_VUT_POLICY}   ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_mtr_VUT.xml
${BASE_VUT_POLICY}   ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_only_VUT.xml
${SULDOWNLOADER_LOG_PATH}           ${SOPHOS_INSTALL}/logs/base/suldownloader.log

*** Test Cases ***
UpdateScheduler Delayed Updating
    [Tags]  UPDATE_SCHEDULER
    [Setup]  Setup For Test With Warehouse Containing Base
    Send Policy To UpdateScheduler  ALC_policy_delayed_updating.xml
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Log Contains   Scheduling updates for Sunday 12:00    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log   Update Scheduler Log

UpdateScheduler Should Fail if Warehouse Does not Have Required Feature
    [Setup]  Setup For Test With Warehouse Containing Base
    ${eventPath} =  Send Policy With Host Redirection And Run Update And Return Event Path     features=SENSORS  remove_subscriptions=MDR
    ${content} =   Get File  ${eventPath}
    Should Contain   ${content}  <number>113</number>   msg=Error does not contain missing package
    Check Sensors Not Installed


UpdateScheduler Install Base and MDR With the ALC Policy With MDR
    [Tags]  SULDOWNLOADER  UPDATE_SCHEDULER  MDR_PLUGIN
    [Setup]  Setup For Test With Warehouse Containing Base and MDR
    Remove Files In Directory  /opt/sophos-spl/base/mcs/event/
    Send Policy With Host Redirection And Run Update And Check Success Of Oldest Event     add_features=MDR   remove_subscriptions=SENSORS
    Check MDR Installed
    Check ALC Status Sent To Central Contains MDR Subscription


*** Keywords ***
Upgrade Installs Product Twice   
    Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log    Installing product: ServerProtectionLinux-Base   2