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
    Mark On Access Log
    Start On Access With Running Threat Detector
    Enable OA Scanning
    Register Cleanup  Require No Unhandled Exception
    Register Cleanup  Check For Coredumps  ${TEST NAME}
    Register Cleanup  Check Dmesg For Segfaults
    Register Cleanup  Exclude CustomerID Failed To Read Error

    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${None}
    Set Suite Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${None}

On Access Test Teardown
    List AV Plugin Path
    Stop On Access
    run teardown functions

    Check All Product Logs Do Not Contain Error

    Dump Log On Failure   ${ON_ACCESS_LOG_PATH}
    Dump Log On Failure   ${THREAT_DETECTOR_LOG_PATH}

    Component Test TearDown

Start On Access With Running Threat Detector
    Start On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Register Cleanup  FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

Start On Access
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
    Wait Until On Access Log Contains With Offset  On-access enabled: "true"
    Wait Until On Access Log Contains With Offset  Starting eventReader

*** Test Cases ***

On Access Log Rotates
    Register Cleanup    Exclude On Access Scan Errors
    Dump and Reset Logs
    # Ensure the log is created
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

    ${pid} =  Get Robot Pid
    ${filepath} =  Set Variable  /tmp_test/eicar.com
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}
    Wait Until On Access Log Contains With Offset  On-close event for ${filepath} from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans A File When It Is Opened
    Mark On Access Log

    ${pid} =  Get Robot Pid
    ${filepath} =  Set Variable  ${RESOURCES_PATH}/file_samples/eicar.iso
    ${Contents} =  Get File  ${filepath}  encoding_errors=ignore
    Wait Until On Access Log Contains With Offset  On-open event for ${filepath} from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  is infected with

On Access Scans File Created By non-root User
    Mark On Access Log

    Create File  /tmp/eicar     ${EICAR_STRING}

    ${command} =    Set Variable    cp /tmp/eicar /tmp/eicar_oa_test
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}

    Log   ${output}

    Wait Until On Access Log Contains With Offset  On-close event for /tmp/eicar_oa_test from PID
    Wait Until On Access Log Contains With Offset  On-open event for /tmp/eicar_oa_test from PID
    Wait Until On Access Log Contains With Offset Times Detected "/tmp/eicar_oa_test" is infected with EICAR-AV-Test  times=2
    On Access Does Not Log Contain  On-close event for /tmp/eicar_oa_test from PID 0 and UID 0
    On Access Does Not Log Contain  On-open event for /tmp/eicar_oa_test from PID 0 and UID 0

On Access Scans File Created Under A Long Path
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  long_dir_eicar  ${EICAR_STRING}

    Wait Until On Access Log Contains With Offset Times  long_dir_eicar" is infected with EICAR-AV-Test   times=2

    Register Cleanup   Remove Directory   /home/vagrant/${LONG_DIRECTORY}   recursive=True
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  silly_long_dir_eicar  ${EICAR_STRING}

    Wait Until On Access Log Contains  Failed to get path from fd: File name too long
    On Access Does Not Log Contain     silly_long_dir_eicar

On Access Scans Encoded Eicars
    Mark AV Log

    Register Cleanup   Remove Directory  /tmp_test/encoded_eicars  true
    ${result} =  Run Process  bash  ${BASH_SCRIPTS_PATH}/createEncodingEicars.sh
    Log Many  ${result.stdout}  ${result.stderr}

    wait_for_all_eicars_are_reported_in_av_log  /tmp_test/encoded_eicars    60

On Access Scans Password Protected File
    Register Cleanup    Exclude As Password Protected

    Mark AV Log

    Register Cleanup   Remove File  /tmp/passwd-protected.xls
    Copy File  ${RESOURCES_PATH}/file_samples/passwd-protected.xls  /tmp/

    Wait Until On Access Log Contains With Offset Times  passwd-protected.xls as it is password protected   times=2

On Access Scans Corrupted File
    Register Cleanup     Exclude As Corrupted

    Mark AV Log

    Register Cleanup   Remove File  /tmp/corrupted.xls
    Copy File  ${RESOURCES_PATH}/file_samples/corrupted.xls  /tmp/

    Wait Until On Access Log Contains With Offset Times  corrupted.xls as it is corrupted   times=2

On Access Scans File On BFS
    Require Filesystem  bfs

    ${image} =  Copy And Extract Image  bfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  bfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset Times  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  times=2

On Access Scans File On CRAMFS
    [Tags]  MANUAL
    # TODO: Fix this test
    Require Filesystem  cramfs

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

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset Times  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  times=2

On Access Scans File On EXT2
    Require Filesystem  ext2

    ${image} =  Copy And Extract Image  ext2FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext2
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset Times  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  times=2

On Access Scans File On EXT3
    Require Filesystem  ext3

    ${image} =  Copy And Extract Image  ext3FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext3
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset Times  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  times=2

On Access Scans File On EXT4
    Require Filesystem  ext4

    ${image} =  Copy And Extract Image  ext4FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext4
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset Times  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  times=2

On Access Scans File On MINIX
    Require Filesystem  minix

    ${image} =  Copy And Extract Image  minixFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  minix
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset Times  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  times=2

On Access Scans File On MSDOS
    Require Filesystem  msdos

    ${image} =  Copy And Extract Image  msdosFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  msdos
    ${opts} =  Set Variable  loop,umask=0000
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset Times  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  times=2

On Access Scans File On NTFS
    Require Filesystem  ntfs

    ${image} =  Copy And Extract Image  ntfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ntfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset Times  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  times=2

On Access Scans File On ReiserFS
    Require Filesystem  reiserfs

    ${image} =  Copy And Extract Image  reiserfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  reiserfs
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset Times  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  times=2

On Access Scans File On SquashFS
    [Tags]  MANUAL
    # TODO: Fix this test
    Require Filesystem  squashfs

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

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset Times  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  times=2

On Access Scans File On VFAT
    Require Filesystem  vfat

    ${image} =  Copy And Extract Image  vfatFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  vfat
    ${opts} =  Set Variable  loop,umask=0000
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset Times  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  times=2

On Access Scans File On XFS
    Require Filesystem  xfs

    ${image} =  Copy And Extract Image  xfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  xfs
    ${opts} =  Set Variable  nouuid
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    Wait Until On Access Log Contains  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset Times  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  times=2


On Access Logs Events Following Write After Being Disabled
    Start Fake Management If Required
    Exclude On Access Connect Failed

    ${enabledPolicyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    ${disabledPolicyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_disabled.xml

    Send Plugin Policy  av  sav  ${disabledPolicyContent}
    Wait Until On Access Log Contains  On-access enabled: "false"

    Send Plugin Policy  av  sav  ${enabledPolicyContent}
    Wait Until On Access Log Contains  On-access enabled: "true"

    Wait Until On Access Log Contains  Starting eventReader
    ${pid} =  Get Robot Pid
    ${filepath} =  Set Variable  /tmp_test/eicar.com
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    Wait Until On Access Log Contains With Offset  On-close event for ${filepath} from PID ${pid} and UID 0
    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from PID ${pid} and UID 0


On Access Doesnt Scan AV Process Events
    ${AVPLUGIN_PID} =  Record AV Plugin PID

    Create File  ${NORMAL_DIRECTORY}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${NORMAL_DIRECTORY}/eicar.com

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}

    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}    Detected "${NORMAL_DIRECTORY}/eicar.com" is infected with EICAR-AV-Test

    Wait Until On Access Log Contains With Offset  Excluding SPL-AV process
    On Access Log Does Not Contain With Offset  from ${AVPLUGIN_PID}