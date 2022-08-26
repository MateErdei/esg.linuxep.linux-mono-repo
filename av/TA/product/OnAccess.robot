*** Settings ***
Documentation   Product tests for SOAP
Force Tags      PRODUCT  SOAP

Library         Process
Library         OperatingSystem
Library         String

Library         ../Libs/AVScanner.py
Library         ../Libs/CoreDumps.py
Library         ../Libs/FakeWatchdog.py
Library         ../Libs/LockFile.py
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

On Access Test Teardown
    Dump Log On Failure   ${ON_ACCESS_LOG_PATH}
    Dump Log On Failure   ${THREAT_DETECTOR_LOG_PATH}
    List AV Plugin Path
    Stop On Access
    run teardown functions

    Check All Product Logs Do Not Contain Error
    Component Test TearDown

Start On Access
    Remove Files   /tmp/soapd.stdout  /tmp/soapd.stderr
    ${handle} =  Start Process  ${ON_ACCESS_BIN}   stdout=/tmp/soapd.stdout  stderr=/tmp/soapd.stderr
    Set Test Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${handle}
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}   stdout=/tmp/av.stdout  stderr=/tmp/av.stderr
    Set Test Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Wait Until On Access running

Stop On Access
    ${result} =  Terminate Process  ${ON_ACCESS_PLUGIN_HANDLE}
    ${result} =  Terminate Process  ${AV_PLUGIN_HANDLE}

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
    Wait Until On Access Log Contains  On-access enabled: "true"
    Wait Until On Access Log Contains  Starting eventReader

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

    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

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
    Wait Until On Access Log Contains  Including mount point: /testmnt/nfsshare

    Start Fake Management If Required

    Mark On Access Log
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_excludeRemoteFiles.xml
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until On Access Log Contains  New on-access configuration: {"enabled":"true","excludeRemoteFiles":"true","exclusions":["/mnt/","/uk-filer5/"]}
    Wait Until On Access Log Contains  On-access enabled: "true"
    Wait Until On Access Log Contains  On-access scan network: "false"
    Wait Until On Access Log Contains  On-access exclusions: ["/mnt/","/uk-filer5/"]

    Wait Until On Access Log Contains  OA config changed, re-enumerating mount points
    On Access Does Not Log Contain With Offset  Including mount point: /testmnt/nfsshare

On Access Monitors Addition And Removal Of Mount Points
    Register Cleanup    Exclude On Access Scan Errors
    [Tags]  NFS
    Mark On Access Log
    Start On Access
    Wait Until On Access Log Contains With Offset  Including mount point:
    On Access Does Not Log Contain With Offset  Including mount point: /testmnt/nfsshare
    Sleep  1s
    ${numMountsPreNFSmount} =  Count Lines In Log With Offset  ${ON_ACCESS_LOG_PATH}  Including mount point:  ${ON_ACCESS_LOG_MARK}
    Log  Number of Mount Points: ${numMountsPreNFSmount}

    Mark On Access Log
    ${source} =       Set Variable  /tmp_test/nfsshare
    ${destination} =  Set Variable  /testmnt/nfsshare
    Create Directory  ${source}
    Create Directory  ${destination}
    Create Local NFS Share   ${source}   ${destination}
    Register Cleanup  Remove Local NFS Share   ${source}   ${destination}

    Wait Until On Access Log Contains With Offset  Mount points changed - re-evaluating
    Wait Until On Access Log Contains With Offset  Including mount point: /testmnt/nfsshare
    Sleep  1s
    ${totalNumMountsPostNFSmount} =  Count Lines In Log With Offset  ${ON_ACCESS_LOG_PATH}  Including mount point:  ${ON_ACCESS_LOG_MARK}
    Log  Number of Mount Points: ${totalNumMountsPostNFSmount}
    Should Be Equal As Integers  ${totalNumMountsPostNFSmount}  ${numMountsPreNFSmount+1}

    Start Fake Management If Required

    Mark On Access Log
    Remove Local NFS Share   ${source}   ${destination}
    Deregister Cleanup  Remove Local NFS Share   ${source}   ${destination}

    Wait Until On Access Log Contains  Mount points changed - re-evaluating
    On Access Does Not Log Contain With Offset  Including mount point: /testmnt/nfsshare
    Sleep  1s
    ${totalNumMountsPostNFSumount} =  Count Lines In Log With Offset  ${ON_ACCESS_LOG_PATH}  Including mount point:  ${ON_ACCESS_LOG_MARK}
    Log  Number of Mount Points: ${totalNumMountsPostNFSumount}
    Should Be Equal As Integers  ${totalNumMountsPostNFSumount}  ${numMountsPreNFSmount}

On Access Scans A File When It Is Closed Following A Write
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${pid} =  Get Robot Pid
    ${filepath} =  Set Variable  /tmp_test/eicar.com
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}
    Wait Until On Access Log Contains  On-close event for ${filepath} from PID ${pid} and UID 0
    Wait Until On Access Log Contains  is infected with

On Access Scans File Created By non-root User
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Create File  /tmp/eicar     ${EICAR_STRING}

    Enable OA Scanning

    ${command} =    Set Variable    cp /tmp/eicar /tmp/eicar_oa_test
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}

    Log   ${output}

    Wait Until On Access Log Contains  On-close event for /tmp/eicar_oa_test from PID
    Wait Until On Access Log Contains  Detected "/tmp/eicar_oa_test" is infected with EICAR-AV-Test
    On Access Does Not Log Contain  On-close event for /tmp/eicar_oa_test from PID 0 and UID 0

On Access Scans File Created Under A Long Path
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  long_dir_eicar  ${EICAR_STRING}

    Wait Until On Access Log Contains   long_dir_eicar" is infected with EICAR-AV-Test

    Register Cleanup   Remove Directory   /home/vagrant/${LONG_DIRECTORY}   recursive=True
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  silly_long_dir_eicar  ${EICAR_STRING}

    Wait Until On Access Log Contains  Failed to get path from fd: File name too long
    On Access Does Not Log Contain     silly_long_dir_eicar

On Access Scans Encoded Eicars
    Mark On Access Log
    Mark AV Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    Register Cleanup   Remove Directory  /tmp_test/encoded_eicars  true
    ${result} =  Run Process  bash  ${BASH_SCRIPTS_PATH}/createEncodingEicars.sh
    Log Many  ${result.stdout}  ${result.stderr}

    wait_for_all_eicars_are_reported_in_av_log  /tmp_test/encoded_eicars    60

On Access Scans File On BFS
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${image} =  Copy And Extract Image  bfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  bfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains  is infected with

On Access Scans File On CRAMFS
    [Tags]  MANUAL
    # TODO: Fix this test
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${image} =  Copy And Extract Image  cramfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  cramfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    ${command} =    Set Variable    cat ${NORMAL_DIRECTORY}/mount/eicar.com > /dev/null
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}
    Log   ${output}

    Wait Until On Access Log Contains  On-close event for ${where}/eicar.com from PID ${pid} and UID
    Wait Until On Access Log Contains  is infected with

On Access Scans File On EXT2
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${image} =  Copy And Extract Image  ext2FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext2
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains  is infected with

On Access Scans File On EXT3
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${image} =  Copy And Extract Image  ext3FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext3
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains  is infected with

On Access Scans File On EXT4
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${image} =  Copy And Extract Image  ext4FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext4
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains  is infected with

On Access Scans File On MINIX
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${image} =  Copy And Extract Image  minixFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  minix
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains  is infected with

On Access Scans File On MSDOS
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${image} =  Copy And Extract Image  msdosFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  msdos
    ${opts} =  Set Variable  loop,umask=0000
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains  is infected with

On Access Scans File On NTFS
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${image} =  Copy And Extract Image  ntfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ntfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains  is infected with

On Access Scans File On ReiserFS
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${image} =  Copy And Extract Image  reiserfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  reiserfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains  is infected with

On Access Scans File On SquashFS
    [Tags]  MANUAL
    # TODO: Fix this test
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${image} =  Copy And Extract Image  squashfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  squashfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    ${command} =    Set Variable    cat ${NORMAL_DIRECTORY}/mount/eicar.com > /dev/null
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}
    Log   ${output}

    Wait Until On Access Log Contains  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains  is infected with

On Access Scans File On VFAT
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${image} =  Copy And Extract Image  vfatFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  vfat
    ${opts} =  Set Variable  loop,umask=0000
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains  is infected with

On Access Scans File On XFS
    Mark On Access Log
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

    Enable OA Scanning

    ${image} =  Copy And Extract Image  xfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  xfs
    ${opts} =  Set Variable  nouuid
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com
    Wait Until On Access Log Contains  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains  is infected with
