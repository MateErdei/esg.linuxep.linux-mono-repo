*** Settings ***
Documentation   Product tests for SOAP
Force Tags      PRODUCT  SOAP

Library         Process
Library         OperatingSystem
Library         String

Library         ../Libs/AVScanner.py
Library         ../Libs/CoreDumps.py
Library         ../Libs/FakeWatchdog.py
Library         ../Libs/FileUtils.py
Library         ../Libs/LockFile.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/OSUtils.py
Library         ../Libs/LogUtils.py
Library         ../Libs/ThreatReportUtils.py

Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

Test Setup     On Access Test Setup
Test Teardown  On Access Test Teardown

*** Variables ***
${AV_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${AV_LOG_PATH}    ${AV_PLUGIN_PATH}/log/av.log
${TESTTMP}  /tmp_test/SSPLAVTests
${SOPHOS_THREAT_DETECTOR_BINARY_LAUNCHER}  ${SOPHOS_THREAT_DETECTOR_BINARY}_launcher
${ONACCESS_FLAG_CONFIG}  ${AV_PLUGIN_PATH}/var/oa_flag.json
${ONACCESS_CONFIG}  ${AV_PLUGIN_PATH}/var/soapd_config.json

*** Keywords ***

List AV Plugin Path
    Create Directory  ${TESTTMP}
    ${result} =  Run Process  ls  -lR  ${AV_PLUGIN_PATH}  stdout=${TESTTMP}/lsstdout  stderr=STDOUT
    Log  ls -lR: ${result.stdout}
    Remove File  ${TESTTMP}/lsstdout

On Access Test Setup
    Component Test Setup
    Mark Sophos Threat Detector Log
    Register Cleanup  Require No Unhandled Exception
    Register Cleanup  Check For Coredumps  ${TEST NAME}
    Register Cleanup  Check Dmesg For Segfaults
    Register Cleanup  Exclude CustomerID Failed To Read Error
    Register Cleanup  Exclude On Access Connect Failed

    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${None}
    Set Suite Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${None}

On Access Test Teardown
    Dump Log On Failure   ${ON_ACCESS_LOG_PATH}
    Dump Log On Failure   ${THREAT_DETECTOR_LOG_PATH}
    List AV Plugin Path
    Stop On Access
    run teardown functions

    Remove File     ${ONACCESS_FLAG_CONFIG}

    Check All Product Logs Do Not Contain Error
    Component Test TearDown

Start On Access With Running Threat Detector
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

Start On Access
    Mark On Access Log
    Remove Files   /tmp/soapd.stdout  /tmp/soapd.stderr
    ${handle} =  Start Process  ${ON_ACCESS_BIN}   stdout=/tmp/soapd.stdout  stderr=/tmp/soapd.stderr
    Set Test Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${handle}
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}   stdout=/tmp/av.stdout  stderr=/tmp/av.stderr
    Set Test Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Wait Until On Access running

Stop On Access
    Run Keyword If  ${AV_PLUGIN_HANDLE}  Terminate Process  ${ON_ACCESS_PLUGIN_HANDLE}
    Run Keyword If  ${ON_ACCESS_PLUGIN_HANDLE}  Terminate Process  ${AV_PLUGIN_HANDLE}

    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${None}
    Set Suite Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${None}

Verify on access log rotated
    List Directory  ${AV_PLUGIN_PATH}/log/
    Should Exist  ${AV_PLUGIN_PATH}/log/soapd.log.1

Dump and Reset Logs
    Register Cleanup   Remove File      ${AV_PLUGIN_PATH}/log/av.log*
    Register Cleanup   Remove File      ${AV_PLUGIN_PATH}/log/soapd.log*
    Register Cleanup   Dump log         ${AV_PLUGIN_PATH}/log/soapd.log

Enable OA Scanning
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset  On-access enabled: "true"
    Wait Until On Access Log Contains With Offset  Starting eventReader

On-access Scan Eicar
    ${pid} =  Get Robot Pid
    ${filepath} =  Set Variable  /tmp_test/eicar.com
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}
    Wait Until On Access Log Contains With Offset  On-close event for ${filepath} from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On-access No Eicar Scan
    ${pid} =  Get Robot Pid
    ${filepath} =  Set Variable  /tmp_test/uncaught_eicar.com
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}
    On Access Log Does Not Contain  On-close event for ${filepath} from PID ${pid} and UID 0

*** Test Cases ***

On Access Log Rotates
    Register Cleanup    Exclude On Access Scan Errors
    Dump and Reset Logs
    # Ensure the log is created
    Start On Access
    Stop On Access
    Increase On Access Log To Max Size
    Start On Access
    Wait Until Created   ${AV_PLUGIN_PATH}/log/soapd.log.1   timeout=10s
    Stop On Access

    ${result} =  Run Process  ls  -altr  ${AV_PLUGIN_PATH}/log/
    Log  ${result.stdout}

    Verify on access log rotated

On Access Process Parses Policy Config
    Register Cleanup    Exclude On Access Scan Errors
    Start AV
    Start On Access

    Start Fake Management If Required

    Enable OA Scanning

    Wait Until On Access Log Contains With Offset  New on-access configuration: {"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"]}
    Wait Until On Access Log Contains With Offset  On-access enabled: "true"
    Wait Until On Access Log Contains With Offset  On-access scan network: "true"
    Wait Until On Access Log Contains With Offset  On-access exclusions: ["/mnt/","/uk-filer5/"]

On Access Process Parses Flags Config On startup
    Register Cleanup    Exclude On Access Scan Errors
    Start AV

    Start Fake Management If Required

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}
    
    Wait Until Created  ${ONACCESS_FLAG_CONFIG}
    
    Start On Access

    Wait Until On Access Log Contains With Offset   Found Flag config on startup
    Wait Until On Access Log Contains With Offset   Flag is set to not override policy

On Access Process Parses Policy Config On startup
    Register Cleanup    Exclude On Access Scan Errors
    Start AV

    Start Fake Management If Required

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until Created  ${ONACCESS_CONFIG}

    Start On Access

    Wait Until On Access Log Contains  New on-access configuration: {"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"]}
    Wait Until On Access Log Contains  On-access enabled: "true"
    Wait Until On Access Log Contains  On-access scan network: "true"
    Wait Until On Access Log Contains  On-access exclusions: ["/mnt/","/uk-filer5/"]

On Access Does Not Include Remote Files If Excluded In Policy
    Register Cleanup    Exclude On Access Scan Errors
    [Tags]  NFS
    ${source} =       Set Variable  /tmp_test/nfsshare
    ${destination} =  Set Variable  /testmnt/nfsshare
    Create Directory  ${source}
    Create Directory  ${destination}
    Create Local NFS Share   ${source}   ${destination}
    Register Cleanup  Remove Local NFS Share   ${source}   ${destination}

    Start On Access
    Wait Until On Access Log Contains With Offset  Including mount point: /testmnt/nfsshare

    Start Fake Management If Required

    Mark On Access Log
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_excludeRemoteFiles.xml
    Send Plugin Policy  av  sav  ${policyContent}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset  New on-access configuration: {"enabled":"true","excludeRemoteFiles":"true","exclusions":["/mnt/","/uk-filer5/"]}
    Wait Until On Access Log Contains With Offset  On-access enabled: "true"
    Wait Until On Access Log Contains With Offset  On-access scan network: "false"
    Wait Until On Access Log Contains With Offset  On-access exclusions: ["/mnt/","/uk-filer5/"]

    Wait Until On Access Log Contains With Offset  OA config changed, re-enumerating mount points
    On Access Log Does Not Contain With Offset  Including mount point: /testmnt/nfsshare

On Access Monitors Addition And Removal Of Mount Points
    Register Cleanup    Exclude On Access Scan Errors
    Require Filesystem  ext2
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext2
    Mark On Access Log
    Start On Access
    Wait Until On Access Log Contains With Offset  Including mount point:
    On Access Log Does Not Contain With Offset  Including mount point: ${where}
    Sleep  1s
    ${numMountsPreMount} =  get_latest_mount_inclusion_count_from_on_access_log  ${ON_ACCESS_LOG_MARK}

    Mark On Access Log
    ${image} =  Copy And Extract Image  ext2FileSystem
    Mount Image  ${where}  ${image}  ${type}

    Wait Until On Access Log Contains With Offset  Mount points changed - re-evaluating
    Wait Until On Access Log Contains  Including mount point: ${where}
    Sleep  1s
    ${totalNumMountsPostMount} =  get_latest_mount_inclusion_count_from_on_access_log  ${ON_ACCESS_LOG_MARK}
    Should Be Equal As Integers  ${totalNumMountsPostMount}  ${numMountsPreMount+1}

    Mark On Access Log
    Unmount Image  ${where}

    Wait Until On Access Log Contains With Offset  Mount points changed - re-evaluating
    On Access Log Does Not Contain With Offset  Including mount point: ${where}
    Sleep  1s
    ${totalNumMountsPostUmount} =  get_latest_mount_inclusion_count_from_on_access_log  ${ON_ACCESS_LOG_MARK}
    Should Be Equal As Integers  ${totalNumMountsPostUmount}  ${numMountsPreMount}

On Access Scans A File When It Is Closed Following A Write
    Start On Access With Running Threat Detector
    Enable OA Scanning

    On-access Scan Eicar

On Access Scans File Created By non-root User
    Start On Access With Running Threat Detector

    Create File  /tmp/eicar     ${EICAR_STRING}

    Enable OA Scanning

    ${command} =    Set Variable    cp /tmp/eicar /tmp/eicar_oa_test
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}

    Log   ${output}

    Wait Until On Access Log Contains With Offset  On-close event for /tmp/eicar_oa_test from PID
    Wait Until On Access Log Contains With Offset  Detected "/tmp/eicar_oa_test" is infected with EICAR-AV-Test
    On Access Log Does Not Contain  On-close event for /tmp/eicar_oa_test from PID 0 and UID 0

On Access Scans File Created Under A Long Path
    Start On Access With Running Threat Detector
    Enable OA Scanning

    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  long_dir_eicar  ${EICAR_STRING}

    Wait Until On Access Log Contains With Offset   long_dir_eicar" is infected with EICAR-AV-Test

    Register Cleanup   Remove Directory   /home/vagrant/${LONG_DIRECTORY}   recursive=True
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  silly_long_dir_eicar  ${EICAR_STRING}

    Wait Until On Access Log Contains With Offset  Failed to get path from fd: File name too long
    On Access Log Does Not Contain     silly_long_dir_eicar

On Access Scans Encoded Eicars
    Mark AV Log
    Start On Access With Running Threat Detector
    Enable OA Scanning

    Register Cleanup   Remove Directory  /tmp_test/encoded_eicars  true
    ${result} =  Run Process  bash  ${BASH_SCRIPTS_PATH}/createEncodingEicars.sh
    Log Many  ${result.stdout}  ${result.stderr}

    wait_for_all_eicars_are_reported_in_av_log  /tmp_test/encoded_eicars    60

On Access Scans Password Protected File
    Register Cleanup    Exclude As Password Protected

    Mark AV Log
    Start On Access With Running Threat Detector
    Enable OA Scanning

    Register Cleanup   Remove File  /tmp/passwd-protected.xls
    Copy File  ${RESOURCES_PATH}/file_samples/passwd-protected.xls  /tmp/

    Wait Until On Access Log Contains With Offset  passwd-protected.xls as it is password protected

On Access Scans Corrupted File
    Register Cleanup     Exclude As Corrupted

    Mark AV Log
    Start On Access With Running Threat Detector
    Enable OA Scanning

    Register Cleanup   Remove File  /tmp/corrupted.xls
    Copy File  ${RESOURCES_PATH}/file_samples/corrupted.xls  /tmp/

    Wait Until On Access Log Contains With Offset  corrupted.xls as it is corrupted

On Access Scans File On BFS
    Require Filesystem  bfs
    Start On Access With Running Threat Detector

    Enable OA Scanning

    ${image} =  Copy And Extract Image  bfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  bfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans File On CRAMFS
    [Tags]  MANUAL
    # TODO: Fix this test
    Require Filesystem  cramfs
    Start On Access With Running Threat Detector
    Enable OA Scanning

    ${image} =  Copy And Extract Image  cramfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  cramfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    ${command} =    Set Variable    cat ${NORMAL_DIRECTORY}/mount/eicar.com > /dev/null
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}
    Log   ${output}

    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans File On EXT2
    Require Filesystem  ext2
    Start On Access With Running Threat Detector
    Enable OA Scanning

    ${image} =  Copy And Extract Image  ext2FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext2
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans File On EXT3
    Require Filesystem  ext3
    Start On Access With Running Threat Detector
    Enable OA Scanning

    ${image} =  Copy And Extract Image  ext3FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext3
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans File On EXT4
    Require Filesystem  ext4
    Start On Access With Running Threat Detector
    Enable OA Scanning

    ${image} =  Copy And Extract Image  ext4FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext4
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans File On MINIX
    Require Filesystem  minix
    Start On Access With Running Threat Detector
    Enable OA Scanning

    ${image} =  Copy And Extract Image  minixFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  minix
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans File On MSDOS
    Require Filesystem  msdos
    Start On Access With Running Threat Detector
    Enable OA Scanning

    ${image} =  Copy And Extract Image  msdosFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  msdos
    ${opts} =  Set Variable  loop,umask=0000
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans File On NTFS
    Require Filesystem  ntfs
    Start On Access With Running Threat Detector
    Enable OA Scanning

    ${image} =  Copy And Extract Image  ntfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ntfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans File On ReiserFS
    Require Filesystem  reiserfs
    Start On Access With Running Threat Detector
    Enable OA Scanning

    ${image} =  Copy And Extract Image  reiserfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  reiserfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans File On SquashFS
    [Tags]  MANUAL
    # TODO: Fix this test
    Require Filesystem  squashfs
    Start On Access With Running Threat Detector
    Enable OA Scanning

    ${image} =  Copy And Extract Image  squashfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  squashfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    ${command} =    Set Variable    cat ${NORMAL_DIRECTORY}/mount/eicar.com > /dev/null
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}
    Log   ${output}

    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans File On VFAT
    Require Filesystem  vfat
    Start On Access With Running Threat Detector
    Enable OA Scanning

    ${image} =  Copy And Extract Image  vfatFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  vfat
    ${opts} =  Set Variable  loop,umask=0000
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans File On XFS
    Require Filesystem  xfs
    Start On Access With Running Threat Detector
    Enable OA Scanning

    ${image} =  Copy And Extract Image  xfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  xfs
    ${opts} =  Set Variable  nouuid
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with


On Access Logs When A File Is Closed Following Write After Being Disabled
    Register Cleanup    Exclude On Access Scan Errors
    Start On Access
    Start Fake Management If Required
    Exclude On Access Connect Failed

    ${enabledPolicyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    ${disabledPolicyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_disabled.xml

    #also enables the flag
    Enable OA Scanning

    Send Plugin Policy  av  sav  ${disabledPolicyContent}
    Wait Until On Access Log Contains With Offset  On-access enabled: "false"

    Send Plugin Policy  av  sav  ${enabledPolicyContent}
    Wait Until On Access Log Contains With Offset  On-access enabled: "true"

    Wait Until On Access Log Contains With Offset  Starting eventReader
    ${pid} =  Get Robot Pid
    ${filepath} =  Set Variable  /tmp_test/eicar.com
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}
    Wait Until On Access Log Contains With Offset  On-close event for ${filepath} from PID ${pid} and UID 0

On Access Process Handles Consecutive Process Control Requests
    Start On Access With Running Threat Detector

    Start Fake Management If Required

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}
    On Access Log Does Not Contain  Using on-access settings from policy

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}
    Wait Until On Access Log Contains With Offset  New on-access configuration: {"enabled":"true"

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_disabled.xml
    Send Plugin Policy  av  sav  ${policyContent}
    Wait Until On Access Log Contains With Offset  No policy override, following policy settings
    Wait Until On Access Log Contains With Offset  New on-access configuration: {"enabled":"false"

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags.json
    Send Plugin Policy  av  FLAGS  ${policyContent}
    Wait Until On Access Log Contains With Offset  Policy override is enabled, on-access will be disabled

    On-access No Eicar Scan

On Access Is Disabled By Default If No Flags Policy Arrives
    Start On Access With Running Threat Detector

    Start Fake Management If Required

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    On-access No Eicar Scan

On Access Uses Policy Settings If Flags Dont Override Policy
    Start On Access With Running Threat Detector

    Start Fake Management If Required

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset   No policy override, following policy settings
    Wait Until On Access Log Contains With Offset   New on-access configuration: {"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"]}

    On-access Scan Eicar

On Access Is Disabled After it Receives Disable Flags
    Start On Access With Running Threat Detector

    Start Fake Management If Required

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset   No policy override, following policy settings

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until On Access Log Contains With Offset   New on-access configuration: {"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"]}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset   Policy override is enabled, on-access will be disabled
    Wait Until On Access Log Contains With Offset   Stopping the reading of Fanotify events

    On-access No Eicar Scan

    Dump Log  ${on_access_log_path}

On Access Does not Use Policy Setttings If Flags Have Overriden Policy
    Start On Access With Running Threat Detector

    Start Fake Management If Required

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset  Policy override is enabled, on-access will be disabled

    Mark On Access Log
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until On Access Log Contains With Offset   Policy override is enabled, on-access will be disabled

    On-access No Eicar Scan

    Dump Log  ${on_access_log_path}

On Access Process Reconnects To Threat Detector
    Register Cleanup    Exclude On Access Scan Errors
    Start On Access With Running Threat Detector
    Enable OA Scanning

    Mark On Access Log
    ${filepath} =  Set Variable  /tmp_test/clean_file_writer/clean.txt
    ${script} =  Set Variable  ${BASH_SCRIPTS_PATH}/cleanFileWriter.sh
    ${HANDLE} =  Start Process  bash  ${script}  stderr=STDOUT
    Register Cleanup  Terminate Process  ${HANDLE}
    Register Cleanup  Remove Directory  /tmp_test/clean_file_writer/  recursive=True

    Wait Until On Access Log Contains With Offset  On-close event for ${filepath}
    FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Mark On Access Log
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Wait Until On Access Log Contains With Offset  On-close event for ${filepath}
    #Depending on whether a scan is being processed or it is being requested one of these 2 errors should appear
    File Log Contains One of   ${ON_ACCESS_LOG_PATH}  0  Failed to receive scan response  Failed to send scan request

    On Access Log Does Not Contain With Offset  Failed to scan ${filepath}

On Access Scan Times Out When Unable To Connect To Threat Detector
    Register Cleanup    Exclude On Access Scan Errors
    Start On Access
    Enable OA Scanning

    Mark On Access Log
    ${filepath} =  Set Variable  /tmp_test/clean_file_writer/clean.txt
    Create File  ${filepath}  clean
    Register Cleanup  Remove File  ${filepath}

    Wait Until On Access Log Contains With Offset  On-close event for ${filepath}
    Wait Until On Access Log Contains With Offset  Failed to connect to Sophos Threat Detector - retrying after sleep
    Wait Until On Access Log Contains With Offset  Reached total maximum number of connection attempts.  timeout=30
