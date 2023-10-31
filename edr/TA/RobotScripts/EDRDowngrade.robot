*** Settings ***
Documentation    Testing EDR Downgrade Logs

Library         Process
Library         OperatingSystem
Library         Collections

Library         ../Libs/XDRLibs.py
Library         ../Libs/FileSystemLibs.py

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     Install With Base SDDS
Suite Teardown  Uninstall All
Test Setup      Install EDR Directly from SDDS With Fixed Value Queries
Test Teardown   Common Teardown

Force Tags    TAP_PARALLEL2

*** Test Cases ***

EDR Log Files Are Saved When Downgrading
    Stop EDR
    Create Debug Level Logger Config File
    Start EDR
    Check EDR Plugin Installed With Base
    Enable XDR
    Wait Until Keyword Succeeds
    ...  40 secs
    ...  1 secs
    ...  Scheduled Query Log Contains    uptime
    Run Live Query and Return Result
    Downgrade EDR
    # check that the log folder contains the downgrade-backup directory
    Directory Should Exist  ${SOPHOS_INSTALL}/plugins/edr/log/downgrade-backup

    # check that the downgrade-backup directory contains the EDR log files
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/log/downgrade-backup/edr.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/log/downgrade-backup/scheduledquery.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/log/downgrade-backup/livequery.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/edr/log/downgrade-backup/edr_osquery.log
