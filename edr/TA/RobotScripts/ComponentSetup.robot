*** Settings ***
Library         Process
Library         OperatingSystem

Library         ../Libs/FakeManagement.py

*** Keywords ***

Component Test Setup
    Run Keyword And Ignore Error   Empty Directory   ${COMPONENT_ROOT_PATH}/log
    Run Keyword And Ignore Error   Empty Directory   ${SOPHOS_INSTALL}/tmp
    Start Fake Management

Component Test TearDown
    Stop Fake Management
    Terminate All Processes  kill=True
    Run Keyword If Test Failed   Dump Log   ${EDR_LOG_FILE}
    Run Keyword If Test Failed   Dump Log   ${FAKEMANAGEMENT_AGENT_LOG_PATH}

Setup Base And Component
    Mock Base Installation
    Setup Component For Testing


Mock Base Installation
    Remove Directory   ${SOPHOS_INSTALL}   recursive=True
    Create Directory   ${SOPHOS_INSTALL}
    Create Directory   ${SOPHOS_INSTALL}/tmp
    Create Directory   ${SOPHOS_INSTALL}/base/mcs/action
    Create Directory   ${SOPHOS_INSTALL}/var/ipc
    Create Directory   ${SOPHOS_INSTALL}/var/ipc/plugins
    Create File        ${SOPHOS_INSTALL}/base/etc/logger.conf   VERBOSITY=DEBUG
    Run Process   groupadd  sophos-spl-group


Setup Component For Testing
    Copy Directory   ${COMPONENT_SDDS}/files/plugins   ${SOPHOS_INSTALL}
    Create Directory   ${SOPHOS_INSTALL}/plugins/edr/var
    Create Directory   ${SOPHOS_INSTALL}/plugins/edr/etc
    Create Directory   ${SOPHOS_INSTALL}/plugins/edr/log
    Run Process   chmod  +x  ${COMPONENT_BIN_PATH}

Log File If Present
    [Arguments]  ${logfile}
    Run Keyword And Ignore Error  Log File  ${logfile}

Uninstall All
    Log File If Present   /tmp/installer.log
    Log File If Present   ${EDR_LOG_FILE}
    Log File If Present   ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Run Keyword And Ignore Error  Uninstall Base
    File Should Not Exist  /etc/rsyslog.d/rsyslog_sophos-spl.conf

Uninstall Base
    ${result} =   Run Process  bash  ${SOPHOS_INSTALL}/bin/uninstall.sh  --force   timeout=60s
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to uninstall base.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"
