*** Settings ***
Documentation    Test base uninstaller clean up all components

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py

Resource  ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource  ${COMMON_TEST_ROBOT}/EventJournalerResources.robot
Resource  ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource  ${COMMON_TEST_ROBOT}/LiveResponseResources.robot
Resource  ${COMMON_TEST_ROBOT}/ResponseActionsResources.robot
Resource  ${COMMON_TEST_ROBOT}/SchedulerUpdateResources.robot

Default Tags  INSTALLER  EDR_PLUGIN  LIVERESPONSE_PLUGIN  UPDATE_SCHEDULER  SMOKE  RESPONSE_ACTIONS_PLUGIN

*** Test Cases ***
Test Components Shutdown Cleanly
    Require Fresh Install
    Override LogConf File as Global Level  DEBUG
    Wait For Base Processes To Be Running

    Create File    ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [watchdog]\nVERBOSITY=DEBUG\n
    Run Process   systemctl  restart  sophos-spl
    Wait For Base Processes To Be Running


    Install Live Response Directly
    Check Live Response Plugin Installed
    Install Event Journaler Directly
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Event Journaler Installed
    Install Response Actions Directly


    #WARNING installing edr should be the last plugin installed to avoid watchdog going down due to the defect LINUXDAR-3732
    Mark Edr Log
    Install EDR Directly
    Wait For EDR to be Installed
    Restart EDR Plugin    clearLog=True    installQueryPacks=True

    Run Process   systemctl  stop  sophos-spl

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check EDR Log Contains  Plugin Finished


    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains   Plugin Finished   ${SOPHOS_INSTALL}/plugins/liveresponse/log/liveresponse.log   LiveResponseLog

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains   Plugin Finished   ${EVENTJOURNALER_DIR}/log/eventjournaler.log   event journaler log
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains   Plugin Finished   ${RESPONSE_ACTIONS_LOG_PATH}   ra log
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains   Update Scheduler Finished   ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log   UpdateSchedulerLog

