*** Settings ***
Documentation    Test base uninstaller clean up all components

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  ../event_journaler/EventJournalerResources.robot
Resource  ../ra_plugin/ResponseActionsResources.robot
Resource  ../edr_plugin/EDRResources.robot
Resource  ../mdr_plugin/MDRResources.robot
Resource  ../liveresponse_plugin/LiveResponseResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource  ../GeneralTeardownResource.robot

Default Tags  INSTALLER  EDR_PLUGIN  LIVERESPONSE_PLUGIN  MDR_PLUGIN  UPDATE_SCHEDULER  SMOKE  RESPONSE_ACTIONS_PLUGIN

*** Test Cases ***
Test Components Shutdown Cleanly
    # Write rsyslog config now before installing the product so that we don't need to handle the rsyslog restart later
    ${rsyslog_conf_dir_exists} =    Does File Exist    /etc/rsyslog.d
    Run Keyword If    ${rsyslog_conf_dir_exists}    Write Rsyslog Config
    Require Fresh Install
    Override LogConf File as Global Level  DEBUG
    Wait For Base Processes To Be Running

    Create File    ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [mtr]\nVERBOSITY=DEBUG\n[watchdog]\nVERBOSITY=DEBUG\n
    Run Process   systemctl  restart  sophos-spl
    Wait For Base Processes To Be Running

    Wait For Base Processes To Be Running

    Install Live Response Directly
    Check Live Response Plugin Installed
    Install Event Journaler Directly
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check Event Journaler Installed
    Install Response Actions Directly
    Block Connection Between EndPoint And FleetManager
    Install MDR Directly
    Check MDR Component Suite Installed Correctly
    Insert MTR Policy
    Wait for MDR Executable To Be Running

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
    ...  Check Mdr Log Contains   Plugin Finished

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

*** Keywords ***

Write Rsyslog Config
    ${EDR_SDDS_DIR} =  Get SSPL EDR Plugin SDDS
    Copy File    ${EDR_SDDS_DIR}/files/plugins/edr/etc/syslog_configs/rsyslog_sophos-spl.conf    /etc/rsyslog.d/
    ${result} =  Run Process    systemctl  restart  rsyslog
    Log  ${result.stdout}
    Log  ${result.stderr}
