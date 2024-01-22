*** Settings ***
Documentation    A second UpdateSchedulerPlugin suite for load balancing purposes

Test Setup      Setup Current Update Scheduler Environment
Test Teardown   Teardown For Test

Library    Process
Library    OperatingSystem
Library    DateTime
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/FakeSulDownloader.py
Library    ${COMMON_TEST_LIBS}/UpdateSchedulerHelper.py
Library    ${COMMON_TEST_LIBS}/MCSRouter.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/LogControlResources.robot
Resource    ${COMMON_TEST_ROBOT}/SchedulerUpdateResources.robot

Force Tags  TAP_PARALLEL3  SLOW  UPDATE_SCHEDULER
*** Variables ***
${UPDATE_START_TIME_FILE}     ${SOPHOS_INSTALL}/base/update/var/updatescheduler/last_update_start_time.conf
*** Test Cases ***
UpdateScheduler Periodically Run SulDownloader
    [Timeout]    35 minutes
    Set Log Level For Component And Reset and Return Previous Log  updatescheduler   DEBUG
    Setup Plugin Install Failed
    # status is to be created in start up - regardless of running update or not.
    Wait Until Created  ${statusPath}
    #Remove noref status
    Remove File  ${statusPath}
    Remove File     ${UPDATE_START_TIME_FILE}
    Create File     ${UPDATE_START_TIME_FILE}    0
    Run Process  chown    sophos-spl-updatescheduler:sophos-spl-group    ${UPDATE_START_TIME_FILE}
    ${eventPath} =  Check Status and Events Are Created  waitTime=30 minutes  attemptsTime=2 minutes
    Check Event Report Install Failed   ${eventPath}
    File should exist    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json

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

UpdateScheduler does not trigger an update before 10 minutes on startup if the last Update was less than an hour ago
    [Timeout]    15 minutes
    ${update_scheduler_mark} =    mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log

    ${currentTime} =  Get Current Date  result_format=epoch
    Set Log Level For Component And Reset and Return Previous Log  updatescheduler   DEBUG
    Setup Plugin Install Failed
    # status is to be created in start up - regardless of running update or not.
    Wait Until Created  ${statusPath}
    #Remove noref status
    Remove File  ${statusPath}
    Remove File     ${UPDATE_START_TIME_FILE}
    ${time}=    Convert To String    ${currentTime}
    Create File     ${UPDATE_START_TIME_FILE}    ${time}
    Run Process  chown    sophos-spl-updatescheduler:sophos-spl-group    ${UPDATE_START_TIME_FILE}
    wait_for_log_contains_from_mark    ${update_scheduler_mark}    First update will be not be triggered as last update was less than an hour ago    ${660}
*** Keywords ***

Teardown For Test
    Log SystemCtl Update Status
    Run Keyword If Test Failed  Log File  /opt/sophos-spl/tmp/fakesul.log
    Run Keyword If Test Failed  Dump Mcs Router Dir Contents
    Run Keyword And Ignore Error  Move File  /etc/hosts.bk  /etc/hosts
    General Test Teardown