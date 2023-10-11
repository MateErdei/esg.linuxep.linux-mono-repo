*** Settings ***
Library         ../Libs/LogUtils.py
Resource    AVResources.robot

*** Keywords ***
Exclude Scan Now mount point does not exist
    # TODO: Remove once LINUXDAR-4805/5057 are fixed
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Failed to scan "/run/user/0": file/folder does not exist

Exclude Scan Now Found Threats But Aborted With 25
    mark_expected_error_in_log  ${AV_LOG_PATH}  ScanScheduler <> Scan: Scan Now, found threats but aborted with exit code: 25
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  ScanScheduler <> Scan: Scan Now, found threats but aborted with exit code: 25

Exclude Permission Denied Setting Default Values For Susi Startup Settings
    mark_expected_error_in_log  ${AV_LOG_PATH}  Permission denied, setting default values for susi startup settings.

Exclude Failed To Write To UnixSocket Environment Interuption
    mark_expected_error_in_log  ${AV_LOG_PATH}  UnixSocket <> Failed to write Process Control Request to socket. Exception caught: Environment interruption

Exclude Process Died With Signal 11
    [Arguments]  ${exe}
    mark_expected_error_in_log  ${WATCHDOG_LOG}  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/av/sbin/${exe} died with 11
    mark_expected_error_in_log  ${WATCHDOG_LOG}  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/av/sbin/${exe} died with signal 11

Exclude Threat Detector Launcher Died With 11
    Exclude Process Died With Signal 11  sophos_threat_detector_launcher

Exclude Threat Report Server died
    # Failed to write Threat Report Client to socket. Exception caught: Environment interruption
    # Logged if AV plugin goes down while TD trying to send alert
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  UnixSocket <> Failed to write Threat Report Client to socket. Exception caught: Environment interruption
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  UnixSocket <> ThreatReporterClient failed to write to socket. Exception caught: Environment interruption
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  UnixSocket <> Failed toed to write Threat Report Client to socket. Exception caught: Environment interruption
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  UnixSocket <> ThreatReporterClient failed to write to socket. Exception caught: Environment interruption


Exclude Soapd Died With 11
    Exclude Process Died With Signal 11  soapd

Exclude SafeStore Died With 11
    Exclude Process Died With Signal 11  safestore

Exclude Threat Detector Launcher Died
    mark_expected_error_in_log  ${WATCHDOG_LOG}  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector_launcher died with 15

Exclude Threat Detector Launcher Died With 70
    mark_expected_error_in_log  ${WATCHDOG_LOG}  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector_launcher died with 70
    mark_expected_error_in_log  ${WATCHDOG_LOG}  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector_launcher died with exit code 70

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
    mark_expected_error_in_log  ${AV_LOG_PATH}  av <> Not processing remainder of SAV policy as Scheduled Scan configuration invalid

Exclude File Name Too Long For Cloud Scan
    mark_expected_error_in_log  ${CLOUDSCAN_LOG_PATH}  [File name too long]

Exclude UnixSocket Interrupted System Call Error Cloud Scan
    mark_expected_error_in_log  ${CLOUDSCAN_LOG_PATH}  UnixSocket <> Reading socket returned error: Interrupted system call

Exclude UpdateScheduler Fails
    mark_expected_error_in_log  ${UPDATE_SCHEDULER}  updatescheduler <> Update Service (sophos-spl-update.service) failed.

Exclude Threat Detector Process Is Killed With SIGKILL
    mark_expected_error_in_log  ${WATCHDOG_LOG}   ProcessMonitoringImpl <> /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector_launcher died with 9

Exclude Failed To connect To Warehouse Error
    mark_expected_error_in_log  ${SULDOWNLOADER_LOG}  suldownloaderdata <> Failed to connect to the warehouse: 5

Exclude Failed To Acquire Susi Lock
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to acquire lock on /opt/sophos-spl/plugins/av/chroot/var/susi_update.lock
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  ThreatScanner <> Failed to acquire lock on /opt/sophos-spl/plugins/av/chroot/var/susi_update.lock

Exclude Failed To Scan Multiple Files Cloud
    mark_expected_error_in_log  ${CLOUDSCAN_LOG_PATH}  NamedScanRunner <> Failed to scan one or more files due to an error

Exclude Failed To Send Scan Request To STD
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  UnixSocket <> Failed to send scan request to Sophos Threat Detector (Environment interruption) - retrying after sleep

Exclude Failed To Scan Files
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Failed to scan
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  [reached the maximum number of connection reconnection attempts. Aborting scan.]
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Failed to scan one or more files due to an error
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  Failed to scan /opt/test/inputs/test_scripts/resources/file_samples/
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  Failed to scan ${RESOURCES_PATH}/file_samples/

Exclude Aborted Scan Errors
    mark_expected_error_in_log  ${ON_ACCESS_LOG_PATH}  OnAccessImpl <> Aborting scan, scanner is shutting down
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Aborting scan, scanner is shutting down
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  UnixSocket <> Aborting scan, scanner is shutting down
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  UnixSocket <> Aborting scan, scanner is shutting down

Exclude Failed To Scan Special File That Cannot Be Read In Threat Detector Logs
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}   ThreatScanner <> Failed to scan /run/netns/avtest as it could not be read
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  Error logged from SUSI while scanning /run/netns/avtest
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> Failed to scan /run/netns/avtest as it could not be read
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  Error logged from SUSI while scanning /run/netns/avtest

Exclude Failed To Scan Special File That Cannot Be Read
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  NamedScanRunner <> Failed to scan /run/netns/avtest as it could not be read
    Exclude Failed To Scan Special File That Cannot Be Read In Threat Detector Logs

Exclude Unixsocket Failed To Send Scan Request To STD
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  UnixSocket <> Failed to send scan request to Sophos Threat Detector (Environment interruption) - retrying after sleep

Exclude Scan Now Terminated
    mark_expected_error_in_log  ${AV_LOG_PATH}  ScanScheduler <> Scan: Scan Now, terminated with exit code

Exclude As Corrupted
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  as it is corrupted
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  as it is corrupted
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  as it is corrupted
    mark_expected_error_in_log  ${ON_ACCESS_LOG_PATH}  as it is corrupted
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}  as it is corrupted

Exclude As Password Protected
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  as it is password protected
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  as it is password protected
    mark_expected_error_in_log  ${SCANNOW_LOG_PATH}  as it is password protected
    mark_expected_error_in_log  ${ON_ACCESS_LOG_PATH}  as it is password protected

Exclude As Zip Bomb
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  as it is a Zip Bomb
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  as it is a Zip Bomb

Exclude Parse Xml Error
    mark_expected_error_in_log  ${AV_LOG_PATH}  av <> Exception caught from plugin at top level (std::runtime_error): Error parsing xml: syntax error

Exclude Scan As Invalid
    mark_expected_error_in_log  ${AV_LOG_PATH}  ScanScheduler <> Refusing to run invalid scan: INVALID

Exclude Safestore connection errors
    mark_expected_error_in_log  ${AV_LOG_PATH}  UnixSocket <> Aborting SafeStore connection : failed to read length

Exclude MCS Router is dead
    mark_expected_error_in_log  ${WATCHDOG_LOG}  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1
    mark_expected_error_in_log  ${WATCHDOG_LOG}  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with exit code 1

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

Threat Detector Failed to Copy
    Exclude MachineID Failed To Read Error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       SophosThreatDetectorImpl <> Failed to copy:
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  SophosThreatDetectorImpl <> Failed to copy:

Exclude EndpointID Cannot Be Empty
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  ThreatScanner <> EndpointID cannot be empty
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ThreatScanner <> EndpointID cannot be empty

Exclude Failed To Create Symlink
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  LogSetup <> Failed to create symlink for logs at /opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/plugins/av/log/sophos_threat_detector: 13
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  LogSetup <> Failed to create symlink for logs at /opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/plugins/av/log/sophos_threat_detector: 13

Exclude Communication Between AV And Base Due To No Incoming Data
    mark_expected_error_in_log  ${MANAGEMENT_AGENT_LOG_PATH}    managementagent <> Failure on sending message to av. Reason: No incoming data

Exclude ThreatHealthReceiver Was Not Initialised Error
    mark_expected_error_in_log  ${MANAGEMENT_AGENT_LOG_PATH}    managementagent <> ThreatHealthReceiver was not initialised, failed to process Threat Health from: av

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

Exclude Globalrep errors
    Exclude Globalrep Timeout Errors
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       [GR] curl_easy_perform() failed: Couldn't connect to server
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  [GR] curl_easy_perform() failed: Couldn't connect to server
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       Error logged from SUSI while scanning
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  Error logged from SUSI while scanning


Exclude SUSI failed to read stream
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}       Failed to read stream:
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  Failed to read stream:

Exclude SUSI Illegal seek error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}     Failed to seek stream: Illegal seek
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}         SUSI error 0xc000000f
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}    SUSI error 0xc000000f
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}     Failed to seek stream: Illegal seek

Exclude On Access Scan Errors
    mark_expected_error_in_log  ${ON_ACCESS_LOG_PATH}           Failed to scan
    mark_expected_error_in_log  ${ON_ACCESS_LOG_PATH}           fanotifyhandler <> Failed to add scan request to queue

#Use only if you trigger this issue
Exclude VDL Folder Missing Errors
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  Package not found: vdl: SUSI error 0xc0000015
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  Failed to install SUSI core data.
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  Failed to bootstrap SUSI with error: -1073741803
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  ScanningServerConnectionThread aborting scan, failed to initialise SUSI

    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  Package not found: vdl: SUSI error 0xc0000015
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  Failed to install SUSI core data.
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  Failed to bootstrap SUSI with error: -1073741803
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  ScanningServerConnectionThread aborting scan, failed to initialise SUSI

Exclude SafeStore Internal Error On Quarantine
    [Arguments]  ${path_name}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}    Failed to quarantine ${path_name} due to: InternalError

Exclude SafeStore File Open Error On Quarantine
    [Arguments]  ${path_name}
    mark_expected_error_in_log  ${SAFESTORE_LOG_PATH}    Failed to quarantine ${path_name} due to: FileOpenFailed

Exclude ThreatDatabase Failed To Parse Database
    mark_expected_error_in_log   ${AV_LOG_PATH}    Resetting ThreatDatabase as we failed to parse ThreatDatabase on disk with error

Exclude Susi Initialisation Failed Messages On Access Enabled
    mark_expected_error_in_log   ${ON_ACCESS_LOG_PATH}    ScanRequestHandler-0 received error: ScanningServerConnectionThread aborting scan, failed to initialise SUSI: Bootstrapping SUSI failed, exiting
    mark_expected_error_in_log   ${ON_ACCESS_LOG_PATH}    ScanRequestHandler-0 received error: ScanningServerConnectionThread aborting scan, failed to initialise SUSI: Bootstrapping SUSI at re-initialization failed, exiting
    mark_expected_error_in_log   ${ON_ACCESS_LOG_PATH}    ScanRequestHandler-0 received error: ScanningServerConnectionThread aborting scan, failed to initialise SUSI: Second attempt to initialise SUSI failed, exiting

    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  Failed to load GLOBALREP_SetScanType from /opt/sophos-spl/plugins/av/chroot/susi/distribution_version/7aa8d848c1d4b3e8c162f0f8991903f594960952b9c849bd1499246d6f1ab880/libglobalrep.so: SUSI error
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  Failed to open the library /opt/sophos-spl/plugins/av/chroot/susi/distribution_version/4557df8c182775bb41a7ea2eac63f754b142b0a9138eb05fc360e6bfe3d38661/libsophtainer.so: /opt/sophos-spl/plugins/av/chroot/susi/distribution_version/4557df8c182775bb41a7ea2eac63f754b142b0a9138eb05fc360e6bfe3d38661/libsophtainer.so: cannot open shared object file: No such file or directory: SUSI error 0xc0000009
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  Core initialization failed.: SUSI error 0xc0000009
    mark_expected_error_in_log  ${THREAT_DETECTOR_LOG_PATH}  Failed to initialise SUSI: 0xc0000009

    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  Failed to load GLOBALREP_SetScanType from /opt/sophos-spl/plugins/av/chroot/susi/distribution_version/7aa8d848c1d4b3e8c162f0f8991903f594960952b9c849bd1499246d6f1ab880/libglobalrep.so: SUSI error
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  Failed to open the library /opt/sophos-spl/plugins/av/chroot/susi/distribution_version/4557df8c182775bb41a7ea2eac63f754b142b0a9138eb05fc360e6bfe3d38661/libsophtainer.so: /opt/sophos-spl/plugins/av/chroot/susi/distribution_version/4557df8c182775bb41a7ea2eac63f754b142b0a9138eb05fc360e6bfe3d38661/libsophtainer.so: cannot open shared object file: No such file or directory: SUSI error 0xc0000009
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  Core initialization failed.: SUSI error 0xc0000009
    mark_expected_error_in_log  ${THREAT_DETECTOR_INFO_LOG_PATH}  Failed to initialise SUSI: 0xc0000009

    Exclude VDL Folder Missing Errors