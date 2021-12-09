*** Settings ***
Library         ../Libs/LogUtils.py
Resource    AVResources.robot

*** Keywords ***
Mark MCS Router is dead
    mark_expected_error_in_log  ${WATCHDOG_LOG}  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1

Mark STD Symlink Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  LogSetup <> Create symlink for logs at
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  LogSetup <> Create symlink for logs at

Mark Watchdog Log Unable To Open File Error
    mark_expected_error_in_log  ${WATCHDOG_LOG}   ProcessMonitoringImpl <> Output: log4cplus:ERROR Unable to open file:

Mark CustomerID Cannot Be Empty Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> CustomerID cannot be empty
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> CustomerID cannot be empty

Mark CustomerID Cannot Be Empty Space Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> CustomerID cannot contain a empty space
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> CustomerID cannot contain a empty space

Mark CustomerID New Line Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> CustomerID cannot contain a new line
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> CustomerID cannot contain a new line

Mark CustomerID Shoulb Be 32 Hex Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> CustomerID should be 32 hex characters
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> CustomerID should be 32 hex characters

Mark CustomerID Hex Format Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> CustomerID must be in hex format
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> CustomerID must be in hex format

Mark CustomerID Failed To Read Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> Failed to read customerID - using default value
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to read customerID - using default value

Mark MachineID Empty Space Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> EndpointID cannot contain a empty space
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> EndpointID cannot contain a empty space

Mark MachineID Hex Format Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> EndpointID must be in hex format
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> EndpointID must be in hex format

Mark MachineID New Line Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> EndpointID cannot contain a new line
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> EndpointID cannot contain a new line

Mark MachineID Shoulb Be 32 Hex Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> EndpointID should be 32 hex characters
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> EndpointID should be 32 hex characters

Mark MachineID Failed To Read Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> Failed to read machine ID - using default value
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to read machine ID - using default value

Mark MachineID Permission Error
    Mark MachineID Failed To Read Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       SophosThreatDetectorImpl <> Failed to copy: "/opt/sophos-spl/base/etc/machine_id.txt"
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  SophosThreatDetectorImpl <> Failed to copy: "/opt/sophos-spl/base/etc/machine_id.txt"