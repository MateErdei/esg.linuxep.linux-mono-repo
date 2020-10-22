*** Settings ***
Library     OperatingSystem
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py

Resource  ../installer/InstallerResources.robot

Suite Setup  Logging Suite Setup
Suite Teardown  Logging Suite Teardown
Test Setup  Logging Test Setup
Test Teardown  Logging Test Teardown

Default Tags  COMMS  LOGGING  TAP_TESTS
*** Variables ***
${LOGGER_CONF_BASENAME}  logger.conf
${LOGGER_CONF_PATH}  ${SOPHOS_INSTALL}/base/etc/${LOGGER_CONF_BASENAME}
${LOGGER_CONF_BACKUP_PATH}  /tmp/${LOGGER_CONF_BASENAME}
*** Keywords ***
Logging Suite Setup
    Require Fresh Install
    Backup Logger Conf

Logging Suite Teardown
    Require Uninstalled
    Remove File  ${LOGGER_CONF_BACKUP_PATH}

Backup Logger Conf
    Copy File  ${LOGGER_CONF_PATH}  ${LOGGER_CONF_BACKUP_PATH}
Restore Logger Conf
    Copy File  ${LOGGER_CONF_BACKUP_PATH}  ${LOGGER_CONF_PATH}

Logging Test Setup
    Restore Logger Conf
    Stop Watchdog

Logging Test Teardown
    General Test Teardown

Set Logname To Debug And Restart Watchdog
    [Arguments]  ${logbase}
    Stop Watchdog
    Append To File  ${LOGGER_CONF_PATH}  [${logbase}]\n
    Append To File  ${LOGGER_CONF_PATH}  VERBOSITY = DEBUG\n
    Start Watchdog


*** Test Cases ***
Test Comms Network Logging Can Be Set To Debug Individually
    Remove File  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophos-spl-comms/comms_network.log
    Set Logname To Debug And Restart Watchdog  comms_network
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  0.5 secs
    ...  Run Keywords
    ...  Check Comms Component Network Log Contains  DEBUG  AND
    ...  Check Watchdog Log Does Not Contain  DEBUG

Test Comms Network And Another Components Logging Can Be Set To Debug Individually
    Remove File  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophos-spl-comms/comms_network.log
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
    Set Logname To Debug And Restart Watchdog  comms_network
    Set Logname To Debug And Restart Watchdog  watchdog
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  0.5 secs
    ...  Run Keywords
    ...  Check Comms Component Network Log Contains  DEBUG  AND
    ...  Check Watchdog Log Contains  DEBUG  AND
    ...  Check UpdateScheduler Log Does Not Contain  DEBUG

Test Watchdog Logging Can Be Set To Debug Individually
    Remove File  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophos-spl-comms/comms_network.log
    Set Logname To Debug And Restart Watchdog  watchdog

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  0.5 secs
    ...  Run Keywords
    ...  Check Watchdog Log Contains  DEBUG  AND
    ...  Check Comms Component Network Log Does Not Contain  DEBUG

