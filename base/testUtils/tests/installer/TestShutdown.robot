*** Settings ***
Documentation    Test base uninstaller clean up all components

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py

Resource  ../edr_plugin/EDRResources.robot
Resource  ../mdr_plugin/MDRResources.robot
Resource  ../liveresponse_plugin/LiveResponseResources.robot
Resource  ../GeneralTeardownResource.robot

Default Tags  INSTALLER  EDR_PLUGIN  LIVERESPONSE_PLUGIN  MDR_PLUGIN  UPDATE_SCHEDULER

*** Test Cases ***
Test Components Shutdown Cleanly
     Require Fresh Install
     Create File    ${SOPHOS_INSTALL}/base/etc/logger.conf.local   [mtr]\nVERBOSITY=DEBUG\n[watchdog]\nVERBOSITY=DEBUG\n
     Run Process   systemctl  restart  sophos-spl
     Wait Until Keyword Succeeds
     ...  10 secs
     ...  1 secs
     ...  Check Expected Base Processes Are Running

     Install EDR Directly
     Wait For EDR to be Installed

     Install Live Response Directly
     Check Live Response Plugin Installed

     Block Connection Between EndPoint And FleetManager
     Install Directly From Component Suite
     Insert MTR Policy

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
      ...  Check Log Contains   Update Scheduler Finished   ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log   UpdateSchedulerLog



