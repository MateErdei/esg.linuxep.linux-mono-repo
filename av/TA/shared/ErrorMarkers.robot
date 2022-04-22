*** Settings ***
Library         ../Libs/LogUtils.py
Resource    AVResources.robot

*** Keywords ***
Exclude Scan Now Found Threats But Aborted With 25
    mark_expected_error_in_log  ${AV_LOG_PATH}  ScanScheduler <> Scan: Scan Now, found threats but aborted with exit code: 25
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  ScanScheduler <> Scan: Scan Now, found threats but aborted with exit code: 25

Exclude Permission Denied Setting Default Values For Susi Startup Settings
    mark_expected_error_in_log  ${AV_LOG_PATH}  Permission denied, setting default values for susi startup settings.

Exclude Failed To Write To UnixSocket Environment Interuption
    mark_expected_error_in_log  ${AV_LOG_PATH}  UnixSocket <> Failed to write Process Control Request to socket. Exception caught: Environment interruption

Exclude Threat Detector Launcher Died With 11
    mark_expected_error_in_log  ${WATCHDOG_LOG}  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector_launcher died with 11

Exclude Threat Detector Launcher Died
    mark_expected_error_in_log  ${WATCHDOG_LOG}  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector_launcher died with 15

Exclude Threat Detector Launcher Died With 70
    mark_expected_error_in_log  ${WATCHDOG_LOG}  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector_launcher died with 70

Exclude Core Not In Policy Features
    mark_expected_error_in_log  ${UPDATE_SCHEDULER}  updatescheduler <> CORE not in the features of the policy.

Exclude SPL Base Not In Subscription Of The Policy
    mark_expected_error_in_log  ${UPDATE_SCHEDULER}  updatescheduler <> SSPL base product name : ServerProtectionLinux-Base not in the subscription of the policy.

Exclude Configuration Data Invalid
    mark_expected_error_in_log  ${SULDOWNLOADER_LOG}  suldownloaderdata <> Configuration data is invalid

Exclude Invalid Settings No Primary Product
    mark_expected_error_in_log  ${SULDOWNLOADER_LOG}  suldownloaderdata <> Invalid Settings: No primary product provided.

Exclude Invalid Day From Policy
    mark_expected_error_in_log  ${AV_LOG_PATH}  ScanScheduler <> Invalid day from policy:

Exclude File Name Too Long For Cloud Scan
    mark_expected_error_in_log  ${CLOUDSCAN_LOG_PATH}  [File name too long]

Exclude UnixSocket Interrupted System Call Error Cloud Scan
    mark_expected_error_in_log  ${CLOUDSCAN_LOG_PATH}  UnixSocket <> Reading socket returned error: Interrupted system call

Exclude UpdateScheduler Fails
    mark_expected_error_in_log  ${UPDATE_SCHEDULER}  updatescheduler <> Update Service (sophos-spl-update.service) failed.

Exclude Threat Detector Process Is Killed With SIGKILL
    mark_expected_error_in_log  ${WATCHDOG_LOG}   ProcessMonitoringImpl <> /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector_launcher died with 9

Exclude Invalid Day From Policy Error
    mark_expected_error_in_log  ${AV_LOG_PATH}  ScanScheduler <> Invalid day from policy:

Exclude Failed To connect To Warehouse Error
    mark_expected_error_in_log  ${SULDOWNLOADER_LOG}  suldownloaderdata <> Failed to connect to the warehouse: 5

Exclude Failed To Acquire Susi Lock
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to acquire lock on /opt/sophos-spl/plugins/av/chroot/var/susi_update.lock
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  ThreatScanner <> Failed to acquire lock on /opt/sophos-spl/plugins/av/chroot/var/susi_update.lock

Exclude Failed To Scan Multiple Files Cloud
    mark_expected_error_in_log  ${CLOUDSCAN_LOG_PATH}  NamedScanRunner <> Failed to scan one or more files due to an error

Exclude Failed To Read Message From Scanning Server
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  UnixSocket <> Failed to read message from Scanning Server - retrying after sleep

Exclude Failed To Send Scan Request To STD
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  UnixSocket <> Failed to send scan request to Sophos Threat Detector (Environment interruption) - retrying after sleep

Exclude Failed To Scan Files
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Failed to scan file:
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  [Reached total maximum number of reconnection attempts. Aborting scan.]
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Failed to scan one or more files due to an error

Exclude Unixsocket Failed To Send Scan Request To STD
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  UnixSocket <> Failed to send scan request to Sophos Threat Detector (Environment interruption) - retrying after sleep

Exclude Scan Now Terminated
    mark_expected_error_in_log  ${AV_LOG_PATH}  ScanScheduler <> Scan: Scan Now, terminated with exit code

Exclude As Corrupted
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  as it is corrupted
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  as it is corrupted
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  as it is corrupted

Exclude As Password Protected
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  as it is password protected
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  as it is password protected
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  as it is password protected

Exclude As Zip Bomb
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  as it is a Zip Bomb
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  as it is a Zip Bomb

Exclude UnixSocket Environment Interruption Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  UnixSocket <> Exiting Scanning Connection Thread: Environment interruption
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  UnixSocket <> Exiting Scanning Connection Thread: Environment interruption

Exclude Parse Xml Error
    mark_expected_error_in_log  ${AV_LOG_PATH}  av <> Exception caught from plugin at top level (std::runtime_error): Error parsing xml: syntax error

Exclude Scan As Invalid
    mark_expected_error_in_log  ${AV_LOG_PATH}  ScanScheduler <> Refusing to run invalid scan: INVALID

Exclude MCS Router is dead
    mark_expected_error_in_log  ${WATCHDOG_LOG}  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1

Exclude STD Symlink Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  LogSetup <> Create symlink for logs at
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  LogSetup <> Create symlink for logs at

Exclude Watchdog Log Unable To Open File Error
    mark_expected_error_in_log  ${WATCHDOG_LOG}  log4cplus:ERROR Unable to open file

Exclude CustomerID Cannot Be Empty Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> CustomerID cannot be empty
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> CustomerID cannot be empty

Exclude CustomerID Cannot Be Empty Space Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> CustomerID cannot contain a empty space
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> CustomerID cannot contain a empty space

Exclude CustomerID New Line Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> CustomerID cannot contain a new line
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> CustomerID cannot contain a new line

Exclude CustomerID Shoulb Be 32 Hex Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> CustomerID should be 32 hex characters
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> CustomerID should be 32 hex characters

Exclude CustomerID Hex Format Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> CustomerID must be in hex format
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> CustomerID must be in hex format

Exclude CustomerID Failed To Read Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> Failed to read customerID - using default value
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to read customerID - using default value

Exclude MachineID Empty Space Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> EndpointID cannot contain a empty space
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> EndpointID cannot contain a empty space

Exclude MachineID Hex Format Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> EndpointID must be in hex format
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> EndpointID must be in hex format

Exclude MachineID New Line Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> EndpointID cannot contain a new line
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> EndpointID cannot contain a new line

Exclude MachineID Shoulb Be 32 Hex Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> EndpointID should be 32 hex characters
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> EndpointID should be 32 hex characters

Exclude MachineID Failed To Read Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> Failed to read machine ID - using default value
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to read machine ID - using default value

Exclude MachineID Permission Error
    Exclude MachineID Failed To Read Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       SophosThreatDetectorImpl <> Failed to copy: "/opt/sophos-spl/base/etc/machine_id.txt"
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  SophosThreatDetectorImpl <> Failed to copy: "/opt/sophos-spl/base/etc/machine_id.txt"

Exclude EndpointID Cannot Be Empty
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  ThreatScanner <> EndpointID cannot be empty
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> EndpointID cannot be empty

Exclude Failed To Create Symlink
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  LogSetup <> Failed to create symlink for logs at /opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/plugins/av/log/sophos_threat_detector: 13
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  LogSetup <> Failed to create symlink for logs at /opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/plugins/av/log/sophos_threat_detector: 13

Exclude Communication Between AV And Base Due To No Incoming Data
    mark_expected_error_in_log  ${MANAGEMENT_AGENT_LOG_PATH}    managementagent <> Failure on sending message to av. Reason: No incoming data

Exclude Expected Sweep Errors
   mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> Failed to scan /mnt/pandorum/BullseyeLM/BullseyeCoverageLicenseManager due to a sweep failure
   mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to scan /mnt/pandorum/BullseyeLM/BullseyeCoverageLicenseManager due to a sweep failure

Exclude Scan Errors From File Samples
   mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> Failed to scan /opt/test/inputs/test_scripts/resources/file_samples/corrupted.xls as it is corrupted
   mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to scan /opt/test/inputs/test_scripts/resources/file_samples/corrupted.xls as it is corrupted
   mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> Failed to scan /opt/test/inputs/test_scripts/resources/file_samples/passwd-protected.xls as it is password protected
   mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to scan /opt/test/inputs/test_scripts/resources/file_samples/passwd-protected.xls as it is password protected
   mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> Failed to scan /regression/opt/test/inputs/test_scripts/resources/file_samples/corrupted.xls as it is corrupted
   mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to scan /regression/opt/test/inputs/test_scripts/resources/file_samples/corrupted.xls as it is corrupted
   mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       ThreatScanner <> Failed to scan /regression/opt/test/inputs/test_scripts/resources/file_samples/passwd-protected.xls as it is password protected
   mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to scan /regression/opt/test/inputs/test_scripts/resources/file_samples/passwd-protected.xls as it is password protected

Exclude Globalrep Timeout Errors
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       =GR= curl_easy_perform() failed: Timeout was reached
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  =GR= curl_easy_perform() failed: Timeout was reached

Exclude SUSI failed to read stream
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       Failed to read stream:
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  Failed to read stream:
