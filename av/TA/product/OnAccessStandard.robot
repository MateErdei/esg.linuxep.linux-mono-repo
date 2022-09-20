#OnAccessStandard starts OnAccess,AV and Threat Detector at suite start up.
#Tests should assess behaviour while these services are running without disrupting their enabled state.
#For tests that want to disrupt standard behaviour use OnAccessAlternative.

*** Settings ***
Documentation   Product tests for SOAP
Force Tags      PRODUCT  SOAP

Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/OnAccessResources.robot

Suite Setup     On Access Suite Setup
Suite Teardown  On Access Suite Teardown

Test Setup     On Access Test Setup
Test Teardown  On Access Test Teardown

*** Variables ***

${TESTTMP}  /tmp_test/SSPLAVTests


*** Keywords ***

#Keep On Access/AV/ThreatDetector running throughout suite
On Access Suite Setup
    Set Suite Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${None}
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${None}
    Mark Sophos Threat Detector Log
    Mark On Access Log
    Start On Access And AV With Running Threat Detector
    Enable OA Scanning


On Access Suite Teardown
    Terminate On Access And AV
    FakeWatchdog.Stop Sophos Threat Detector Under Fake Watchdog
    #fakewatchdog shut in global teardown
    Remove Files    ${ONACCESS_FLAG_CONFIG}  /tmp/av.stdout  /tmp/av.stderr  /tmp/soapd.stdout  /tmp/soapd.stderr


On Access Test Setup
    Component Test Setup
    Mark On Access Log
    Start On Access If Not Running
    Start Av Plugin If Not Running
    Start Sophos Threat Detector If Not Running
    Register Cleanup  Clear On Access Log When Nearly Full
    Register Cleanup  Require No Unhandled Exception
    Register Cleanup  Check For Coredumps  ${TEST NAME}
    Register Cleanup  Check Dmesg For Segfaults
    Register Cleanup  Exclude CustomerID Failed To Read Error


On Access Test Teardown
    List AV Plugin Path
    run teardown functions
    Check All Product Logs Do Not Contain Error

    Dump Log On Failure   ${ON_ACCESS_LOG_PATH}
    Dump Log On Failure   ${THREAT_DETECTOR_LOG_PATH}


*** Test Cases ***

On Access Scans A File When It Is Closed Following A Write

    On-access Scan Eicar Close


On Access Scans A File When It Is Opened

    On-access Scan Eicar Open


On Access Scans File Created By non-root User

    ${srcfile} =  Set Variable  /tmp/eicar
    ${destfile} =  Set Variable  /tmp/eicar_oa_test
    Create File  ${srcfile}     ${EICAR_STRING}
    Register Cleanup  Remove File  ${srcfile}

    ${command} =    Set Variable    cp ${srcfile} ${destfile}
    Register Cleanup  Remove File  ${destfile}
    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}

    Log   ${output}

    Wait Until On Access Log Contains With Offset  On-close event for ${destfile} from
    Wait Until On Access Log Contains With Offset  Detected "${destfile}" is infected with EICAR-AV-Test  timeout=${timeout}


On Access Scans File Created Under A Long Path
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  long_dir_eicar  ${EICAR_STRING}
    Register Cleanup   Remove Directory   /home/vagrant/${LONG_DIRECTORY}   recursive=True

    Wait Until On Access Log Contains With Offset   long_dir_eicar" is infected with EICAR-AV-Test  timeout=${timeout}

    Mark On Access Log
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  silly_long_dir_eicar  ${EICAR_STRING}

    Wait Until On Access Log Contains With Offset  Failed to get path from fd: File name too long  timeout=${timeout}
    On Access Log Does Not Contain     silly_long_dir_eicar


On Access Scans Encoded Eicars
    Mark AV Log

    Register Cleanup   Remove Directory  /tmp_test/encoded_eicars  true
    ${result} =  Run Process  bash  ${BASH_SCRIPTS_PATH}/createEncodingEicars.sh
    Log Many  ${result.stdout}  ${result.stderr}

    wait_for_all_eicars_are_reported_in_av_log  /tmp_test/encoded_eicars    60

On Access Scans Password Protected File
    Register Cleanup    Exclude As Password Protected
    Mark AV Log

    Copy File  ${RESOURCES_PATH}/file_samples/passwd-protected.xls  /tmp/
    Register Cleanup   Remove File  /tmp/passwd-protected.xls
    #Todo Test can see below message 2-3 times
    Wait Until On Access Log Contains With Offset   /passwd-protected.xls as it is password protected   timeout=${timeout}


On Access Scans Corrupted File
    Register Cleanup     Exclude As Corrupted
    Mark On Access Log
    Mark AV Log

    Copy File  ${RESOURCES_PATH}/file_samples/corrupted.xls  /tmp/
    Register Cleanup   Remove File  /tmp/corrupted.xls
    #Todo Test can see below message 2-3 times
    Wait Until On Access Log Contains With Offset   /corrupted.xls as it is corrupted   timeout=${timeout}


On Access Scans File On BFS
    Require Filesystem  bfs

    ${image} =  Copy And Extract Image  bfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  bfs
    Mark On Access Log
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Mark On Access Log
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from
    Wait Until On Access Log Contains With Offset   Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  timeout=${timeout}

On Access Scans File On CRAMFS
    Require Filesystem  cramfs

    ${image} =  Copy And Extract Image  cramfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  cramfs
    Mark On Access Log
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    ${contents} =  Get Binary File  ${NORMAL_DIRECTORY}/mount/eicar.com

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from
    Wait Until On Access Log Contains With Offset  (PID=${pid}) and UID 0
    Wait Until On Access Log Contains With Offset  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  timeout=${timeout}

On Access Scans File On EXT2
    Require Filesystem  ext2

    ${image} =  Copy And Extract Image  ext2FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext2
    Mark On Access Log
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Mark On Access Log
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from
    Wait Until On Access Log Contains With Offset  (PID=${pid}) and UID 0
    Wait Until On Access Log Contains With Offset   Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  timeout=${timeout}

On Access Scans File On EXT3
    Require Filesystem  ext3

    ${image} =  Copy And Extract Image  ext3FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext3
    Mark On Access Log
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Mark On Access Log
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from
    Wait Until On Access Log Contains With Offset  (PID=${pid}) and UID 0
    Wait Until On Access Log Contains With Offset   Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  timeout=${timeout}

On Access Scans File On EXT4
    Require Filesystem  ext4

    ${image} =  Copy And Extract Image  ext4FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext4
    Mark On Access Log
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Mark On Access Log
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from
    Wait Until On Access Log Contains With Offset  (PID=${pid}) and UID 0
    Wait Until On Access Log Contains With Offset   Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  timeout=${timeout}

On Access Scans File On MINIX
    Require Filesystem  minix

    ${image} =  Copy And Extract Image  minixFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  minix
    Mark On Access Log
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Mark On Access Log
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from
    Wait Until On Access Log Contains With Offset  (PID=${pid}) and UID 0
    Wait Until On Access Log Contains With Offset   Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  timeout=${timeout}

On Access Scans File On MSDOS
    Require Filesystem  msdos

    ${image} =  Copy And Extract Image  msdosFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  msdos
    ${opts} =  Set Variable  loop,umask=0000
    Mark On Access Log
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Mark On Access Log
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from
    Wait Until On Access Log Contains With Offset  (PID=${pid}) and UID 0
    Wait Until On Access Log Contains With Offset   Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  timeout=${timeout}

On Access Scans File On NTFS
    Require Filesystem  ntfs

    ${image} =  Copy And Extract Image  ntfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ntfs
    Mark On Access Log
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Mark On Access Log
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from
    Wait Until On Access Log Contains With Offset  (PID=${pid}) and UID 0
    Wait Until On Access Log Contains With Offset   Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  timeout=${timeout}

On Access Scans File On ReiserFS
    Require Filesystem  reiserfs

    ${image} =  Copy And Extract Image  reiserfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  reiserfs
    Mark On Access Log
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Mark On Access Log
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from
    Wait Until On Access Log Contains With Offset  (PID=${pid}) and UID 0
    Wait Until On Access Log Contains With Offset   Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  timeout=${timeout}

On Access Scans File On SquashFS
    Require Filesystem  squashfs

    ${image} =  Copy And Extract Image  squashfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  squashfs
    Mark On Access Log
    Mount Image  ${where}  ${image}  ${type}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    ${contents} =  Get Binary File  ${NORMAL_DIRECTORY}/mount/eicar.com

    Wait Until On Access Log Contains With Offset  On-open event for ${where}/eicar.com from
    Wait Until On Access Log Contains With Offset  (PID=${pid}) and UID 0
    Wait Until On Access Log Contains With Offset   Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with   timeout=${timeout}

On Access Scans File On VFAT
    Require Filesystem  vfat

    ${image} =  Copy And Extract Image  vfatFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  vfat
    ${opts} =  Set Variable  loop,umask=0000
    Mark On Access Log
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Mark On Access Log
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from
    Wait Until On Access Log Contains With Offset  (PID=${pid}) and UID 0
    Wait Until On Access Log Contains With Offset   Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  timeout=${timeout}

On Access Scans File On XFS
    Require Filesystem  xfs

    ${image} =  Copy And Extract Image  xfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  xfs
    ${opts} =  Set Variable  nouuid
    Mark On Access Log
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    Wait Until On Access Log Contains With Offset  Including mount point: ${NORMAL_DIRECTORY}/mount

    ${pid} =  Get Robot Pid
    Mark On Access Log
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    Wait Until On Access Log Contains With Offset  On-close event for ${where}/eicar.com from
    Wait Until On Access Log Contains With Offset  (PID=${pid}) and UID 0
    Wait Until On Access Log Contains With Offset   Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  timeout=${timeout}


On Access Doesnt Scan AV Process Events
    ${AVPLUGIN_PID} =  Record AV Plugin PID
    ${filepath} =  Set Variable  ${NORMAL_DIRECTORY}/eicar.com

    Mark AV Log

    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}

    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}    Detected "${filepath}" is infected with EICAR-AV-Test

    Wait Until AV Plugin Log Contains With Offset  Found 'EICAR-AV-Test' in '${filepath}
    On Access Log Does Not Contain With Offset  from ${AVPLUGIN_PID}


On Access Caches Open Events Without Detections
    ${cleanfile} =  Set Variable  /tmp_test/cleanfile.txt
    ${dirtyfile} =  Set Variable  /tmp_test/dirtyfile.txt

    Create File   ${cleanfile}   ${CLEAN_STRING}
    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup   Run Process   rm   ${cleanfile}
    Register Cleanup   Run Process   rm   ${dirtyfile}

    Mark On Access Log
    Generate Only Open Event   ${cleanfile}
    Wait Until On Access Log Contains With Offset  On-open event for ${cleanfile} from    timeout=${timeout}

    Mark On Access Log
    Sleep   1  #Let the event be cached
    Generate Only Open Event   ${cleanfile}

    #Generate another event we can expect in logs
    Generate Only Open Event   ${dirtyfile}
    Wait Until On Access Log Contains With Offset  On-open event for ${dirtyfile} from    timeout=${timeout}
    On Access Log Does Not Contain With Offset   On-open event for ${cleanfile} from


On Access Doesnt Cache Open Events With Detections
    ${dirtyfile} =  Set Variable  /tmp_test/dirtyfile.txt

    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup   Run Process   rm   ${dirtyfile}

    Mark On Access Log
    Generate Only Open Event   ${dirtyfile}

    Sleep   1  #Let the event be cached

    Generate Only Open Event   ${dirtyfile}

    Wait Until On Access Log Contains Times With Offset  On-open event for ${dirtyfile} from    timeout=${timeout}    times=2
    Wait Until On Access Log Contains Times With Offset  Detected "${dirtyfile}" is infected with EICAR-AV-Test (Open)   timeout=${timeout}    times=2


On Access Doesnt Cache Close Events
    ${srcfile} =  Set Variable  /tmp_test/cleanfile.txt
    ${destfile} =  Set Variable  /tmp_test_two/cleanfile.txt

    Create File  ${srcfile}  ${CLEAN_STRING}
    Register Cleanup   Run Process   rm   ${srcfile}

    Mark On Access Log
    Copy File No Temp Directory   ${srcfile}   ${destfile}
    Register Cleanup   Run Process   rm   ${destfile}

    Sleep   1  #Let the event (hopefully not) be cached

    Copy File No Temp Directory   ${srcfile}   ${destfile}
    Wait Until On Access Log Contains Times With Offset  On-close event for ${destfile} from    timeout=${timeout}  times=2
    dump_logs