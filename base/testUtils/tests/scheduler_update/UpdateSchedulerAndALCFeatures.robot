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
Resource  ../GeneralTeardownResource.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot


Default Tags   SULDOWNLOADER  UPDATE_SCHEDULER
Force Tags  LOAD6

*** Test Cases ***
#TODO migrate test to SDDS3 - LINUXDAR-6948
UpdateScheduler Delayed Updating
    [Tags]  UPDATE_SCHEDULER  TESTFAILURE
    #Removed [Setup]  Setup For Test With Warehouse Containing Base
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Send Policy To UpdateScheduler  ALC_policy_delayed_updating.xml
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    Scheduling product updates for Sunday at 12:00    ${10}

#TODO migrate test to SDDS3 - LINUXDAR-6948
UpdateScheduler Should Fail if Warehouse is missing multiple packages
    [Tags]   TESTFAILURE
    #Removed [Setup]  Setup For Test With Warehouse Containing Base
    ${eventPath} =  Send Policy With Host Redirection And Run Update And Return Event Path   ALC_policy_direct_local_warehouse.xml   features=LIVEQUERY  remove_subscriptions=MDR
    ${content} =   Get File  ${eventPath}
    Should Contain   ${content}  <number>113</number>   msg=Error does not contain missing package
    Directory Should be empty  ${SOPHOS_INSTALL}/plugins


*** Keywords ***
Upgrade Installs Product Twice   
    Check Log Contains String N Times   ${SULDOWNLOADER_LOG_PATH}   SULDownloader Log    Installing product: ServerProtectionLinux-Base   2
