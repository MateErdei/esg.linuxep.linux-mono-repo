*** Settings ***
Library         ../Libs/LogUtils.py
Resource    AVResources.robot

*** Keywords ***
Mark Failed To Acquire Susi Lock
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to acquire lock on /opt/sophos-spl/plugins/av/chroot/var/susi_update.lock
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  ThreatScanner <> Failed to acquire lock on /opt/sophos-spl/plugins/av/chroot/var/susi_update.lock


Mark Failed To Scan Files
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Failed to scan file: /run/motd.dynamic after 60 retries
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Failed to scan file: /run/mlocate.daily.lock after 60 retries
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Failed to scan file: /run/agetty.reload after 60 retries
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Failed to scan file: /run/vboxadd-service.sh after 60 retries
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Failed to scan/run/sshd.pid [Reached total maximum number of reconnection attempts. Aborting scan.]
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Failed to scan one or more files due to an error

Mark Unixsocket Failed To Send Scan Request To STD
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  UnixSocket <> Failed to send scan request to Sophos Threat Detector (Environment interruption) - retrying after sleep

Mark Scan Now Terminated
    mark_expected_error_in_log  ${AV_LOG_PATH}  ScanScheduler <> Scan: Scan Now, terminated with exit code: 36

Mark As Corrupted
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  as it is corrupted
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  as it is corrupted

Mark As Password Protected
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  as it is password protected
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  as it is password protected

Mark As Zip Bomb
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  as it is a Zip Bomb
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  as it is a Zip Bomb

Mark UnixSocket Failed To Read Length
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  UnixSocket <> Aborting Scanning Server Connection Thread: failed to read length
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  UnixSocket <> Aborting Scanning Server Connection Thread: failed to read length

Mark UnixSocket Connection Reset By Peer
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  UnixSocket <> Reading socket returned error: Connection reset by peer
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  UnixSocket <> Reading socket returned error: Connection reset by peer

Mark UnixSocket Environment Interruption Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  UnixSocket <> Exiting Scanning Connection Thread: Environment interruption
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  UnixSocket <> Exiting Scanning Connection Thread: Environment interruption

Mark Parse Xml Error
    mark_expected_error_in_log  ${AV_LOG_PATH}  av <> Exception caught from plugin at top level (std::runtime_error): Error parsing xml: syntax error

Mark Scan As Invalid
    mark_expected_error_in_log  ${AV_LOG_PATH}  ScanScheduler <> Refusing to run invalid scan: INVALID

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