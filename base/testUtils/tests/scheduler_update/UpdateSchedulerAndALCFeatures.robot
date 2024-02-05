*** Settings ***
Documentation    Check UpdateScheduler Plugin using real SulDownloader and the Fake Warehouse to prove it works.

Test Teardown   Teardown Servers For Update Scheduler
Test Setup  Setup Update Scheduler Environment

Library    Process
Library    OperatingSystem
Library    ${COMMON_TEST_LIBS}/UpdateSchedulerHelper.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/LogControlResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/SchedulerUpdateResources.robot

Force Tags   SULDOWNLOADER  UPDATE_SCHEDULER  TAP_PARALLEL3

*** Test Cases ***
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
