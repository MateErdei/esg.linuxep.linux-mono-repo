*** Settings ***
Documentation    A second UpdateSchedulerPlugin suite for load balancing purposes

Test Setup      Setup Current Update Scheduler Environment
Test Teardown   Teardown For Test

Library    Process
Library    OperatingSystem
Library    DateTime
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${LIBS_DIRECTORY}/FakeSulDownloader.py
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/LogControlResources.robot
Resource    ${COMMON_TEST_ROBOT}/SchedulerUpdateResources.robot

Default Tags  SLOW  UPDATE_SCHEDULER
Force Tags  LOAD8

*** Test Cases ***
UpdateScheduler Periodically Run SulDownloader
    [Timeout]    35 minutes
    Set Log Level For Component And Reset and Return Previous Log  updatescheduler   DEBUG
    Setup Plugin Install Failed
    # status is to be created in start up - regardless of running update or not.
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Exist  ${statusPath}
    #Remove noref status
    Remove File  ${statusPath}

    ${eventPath} =  Check Status and Events Are Created  waitTime=30 minutes  attemptsTime=2 minutes
    Check Event Report Install Failed   ${eventPath}

    Remove File  ${eventPath}
    #Remove status with the products in
    Remove File  ${statusPath}
    @{features}=  Create List   CORE
    Setup Base and Plugin Upgraded  {$features}  startTime=3  syncTime=3
    #Wait for up to 30 mins for the update

    ${eventPath} =  Check Event File Generated   1800
    Check Event Report Success  ${eventPath}

    # After first succesful update feature codes are added to the ALC status
    File Should Exist  ${statusPath}

*** Keywords ***

Teardown For Test
    Log SystemCtl Update Status
    Run Keyword If Test Failed  Log File  /opt/sophos-spl/tmp/fakesul.log
    Run Keyword If Test Failed  Dump Mcs Router Dir Contents
    Run Keyword And Ignore Error  Move File  /etc/hosts.bk  /etc/hosts
    General Test Teardown