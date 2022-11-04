#OnAccessStandard starts OnAccess,AV and Threat Detector at suite start up.
#Tests should assess behaviour while these services are running without disrupting their enabled state.
#For tests that want to disrupt standard behaviour use OnAccessAlternative.

*** Settings ***
Documentation   Product tests for SOAP
Force Tags      PRODUCT  SOAP

Resource    ../shared/AVResources.robot
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/FakeManagementResources.robot

Resource    ../shared/OnAccessResources.robot

Suite Setup     On Access Suite Setup
Suite Teardown  On Access Suite Teardown

Test Setup     On Access Test Setup
Test Teardown  On Access Test Teardown

*** Variables ***

${TESTTMP}  /tmp_test/SSPLAVTests


*** Keywords ***
On Access Suite Setup
    Set Suite Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${None}
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${None}
    ${customerIdFile} =   Set Variable   ${AV_PLUGIN_PATH}/var/customer_id.txt
    Create File  ${customerIdFile}  c1cfcf69a42311a6084bcefe8af02c8a
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
    Register Cleanup  Check All Product Logs Do Not Contain Error
    Register Cleanup  Exclude On Access Scan Errors
    Register Cleanup  Require No Unhandled Exception
    Register Cleanup  Check For Coredumps  ${TEST NAME}
    Register Cleanup  Check Dmesg For Segfaults
    Register Cleanup  Exclude CustomerID Failed To Read Error


On Access Test Teardown
    List AV Plugin Path
    run teardown functions

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

    ${mark} =  get_on_access_log_mark

    ${su_command} =    Set Variable    su -s /bin/sh -c "${command}" nobody
    ${rc}   ${output} =    Run And Return Rc And Output   ${su_command}

    Log   ${output}

    wait_for_on_access_log_contains_after_mark  On-close event for ${destfile} from  mark=${mark}  timeout=${timeout}
    wait_for_on_access_log_contains_after_mark  Detected "${destfile}" is infected with EICAR-AV-Test  mark=${mark}  timeout=${timeout}


On Access Scans File Created Under A Long Path
    ${mark} =  get_on_access_log_mark
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${40}  /home/vagrant/  long_dir_eicar  ${EICAR_STRING}
    Register Cleanup   Remove Directory   /home/vagrant/${LONG_DIRECTORY}   recursive=True

    wait_for_on_access_log_contains_after_mark  long_dir_eicar" is infected with EICAR-AV-Test  mark=${mark}  timeout=${timeout}

    ${mark} =  get_on_access_log_mark
    ${long_path} =  create long path  ${LONG_DIRECTORY}   ${100}  /home/vagrant/  silly_long_dir_eicar  ${EICAR_STRING}

    wait_for_on_access_log_contains_after_mark  Failed to get path from fd: File name too long  mark=${mark}  timeout=${timeout}
    check_on_access_log_does_not_contain_after_mark   silly_long_dir_eicar  mark=${mark}


On Access Scans Encoded Eicars
    ${mark} =  get_av_log_mark

    Register Cleanup   Remove Directory  /tmp_test/encoded_eicars  true
    ${result} =  Run Process  bash  ${BASH_SCRIPTS_PATH}/createEncodingEicars.sh
    Log Many  ${result.stdout}  ${result.stderr}

    wait_for_all_eicars_are_reported_in_av_log  /tmp_test/encoded_eicars   mark=${mark}  timeout=${60}

On Access Scans Password Protected File
    Register Cleanup    Exclude As Password Protected
    ${mark} =  get_on_access_log_mark

    Copy File  ${RESOURCES_PATH}/file_samples/passwd-protected.xls  /tmp/
    Register Cleanup   Remove File  /tmp/passwd-protected.xls
    #Todo Test can see below message 2-3 times
    wait_for_on_access_log_contains_after_mark   /passwd-protected.xls as it is password protected  mark=${mark}  timeout=${timeout}


On Access Scans Corrupted File
    Register Cleanup     Exclude As Corrupted
    ${mark} =  get_on_access_log_mark

    Copy File  ${RESOURCES_PATH}/file_samples/corrupted.xls  /tmp/
    Register Cleanup   Remove File  /tmp/corrupted.xls
    #Todo Test can see below message 2-3 times
    wait_for_on_access_log_contains_after_mark   /corrupted.xls as it is corrupted  mark=${mark}  timeout=${timeout}


On Access Scans File On BFS
    Require Filesystem  bfs

    ${image} =  Copy And Extract Image  bfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  bfs
    ${mark} =  get_on_access_log_mark
    Mount Image  ${where}  ${image}  ${type}
    wait for on access log contains after mark  Including mount point: ${NORMAL_DIRECTORY}/mount  mark=${mark}

    ${pid} =  Get Robot Pid
    ${mark} =  get_on_access_log_mark
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    wait for on access log contains after mark  On-close event for ${where}/eicar.com from  mark=${mark}
    wait for on access log contains after mark  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  mark=${mark}  timeout=${timeout}

On Access Scans File On CRAMFS
    Require Filesystem  cramfs

    ${image} =  Copy And Extract Image  cramfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  cramfs
    ${mark} =  get_on_access_log_mark
    Mount Image  ${where}  ${image}  ${type}
    wait for on access log contains after mark  Including mount point: ${NORMAL_DIRECTORY}/mount  mark=${mark}

    ${pid} =  Get Robot Pid
    ${contents} =  Get Binary File  ${NORMAL_DIRECTORY}/mount/eicar.com

    wait for on access log contains after mark  On-open event for ${where}/eicar.com from  mark=${mark}
    wait for on access log contains after mark  (PID=${pid}) and UID 0  mark=${mark}
    wait for on access log contains after mark  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  mark=${mark}  timeout=${timeout}

On Access Scans File On EXT2
    Require Filesystem  ext2

    ${image} =  Copy And Extract Image  ext2FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext2
    ${mark} =  get_on_access_log_mark
    Mount Image  ${where}  ${image}  ${type}
    wait for on access log contains after mark  Including mount point: ${NORMAL_DIRECTORY}/mount  mark=${mark}

    ${pid} =  Get Robot Pid
    ${mark} =  get_on_access_log_mark
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    wait for on access log contains after mark  On-close event for ${where}/eicar.com from  mark=${mark}
    wait for on access log contains after mark  (PID=${pid}) and UID 0  mark=${mark}
    wait for on access log contains after mark  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  mark=${mark}  timeout=${timeout}

On Access Scans File On EXT3
    Require Filesystem  ext3

    ${image} =  Copy And Extract Image  ext3FileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    ${type} =  Set Variable  ext3
    ${mark} =  get_on_access_log_mark
    Mount Image  ${where}  ${image}  ${type}
    wait for on access log contains after mark  Including mount point: ${NORMAL_DIRECTORY}/mount  mark=${mark}

    ${pid} =  Get Robot Pid
    ${mark} =  get_on_access_log_mark
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    wait for on access log contains after mark  On-close event for ${where}/eicar.com from  mark=${mark}
    wait for on access log contains after mark  (PID=${pid}) and UID 0  mark=${mark}
    wait for on access log contains after mark  Detected "/home/vagrant/this/is/a/directory/for/scanning/mount/eicar.com" is infected with  mark=${mark}  timeout=${timeout}

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


On Access Includes Included Mount On Top Of Excluded Mount
    ${excludedMount} =  Set Variable  /proc/tty
    ${includedMount} =  Set Variable  ${NORMAL_DIRECTORY}/test
    ${dest} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    Directory Should Exist  ${excludedMount}
    Create Directory  ${includedMount}

    Mark On Access Log
    Bind Mount Directory  ${excludedMount}  ${dest}
    Wait Until On Access Log Contains With Offset  Mount point ${dest} is system and will be excluded from the scan
    Bind Mount Directory  ${includedMount}  ${dest}
    Wait Until On Access Log Contains With Offset  Including mount point: ${dest}


On Access Excludes Excluded Mount On Top Of Included Mount
    ${excludedMount} =  Set Variable  /proc/tty
    ${includedMount} =  Set Variable  ${NORMAL_DIRECTORY}/test
    ${dest} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    Directory Should Exist  ${excludedMount}
    Create Directory  ${includedMount}

    Mark On Access Log
    Bind Mount Directory  ${includedMount}  ${dest}
    Wait Until On Access Log Contains With Offset  Including mount point: ${dest}
    Bind Mount Directory  ${excludedMount}  ${dest}
    Wait Until On Access Log Contains With Offset  Mount point ${dest} is system and will be excluded from the scan


On Access Doesnt Scan Threat Detector Events
    ${TD_PID} =  Record Sophos Threat Detector PID
    ${filepath} =  Set Variable  ${NORMAL_DIRECTORY}/eicar.com

    Mark On Access Log

    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    Wait Until On Access Log Contains With Offset   Detected "${filepath}" is infected with EICAR-AV-Test  timeout=${timeout}
    On Access Log Does Not Contain With Offset   from Process ${SOPHOS_THREAT_DETECTOR_BINARY}(PID=${TD_PID})


On Access Doesnt Scan CommandLine Scanner Events
    ${filepath} =  Set Variable  ${NORMAL_DIRECTORY}/eicar.com

    Mark On Access Log

    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}

    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}    Detected "${filepath}" is infected with EICAR-AV-Test

    Wait Until On Access Log Contains With Offset   Detected "${filepath}" is infected with EICAR-AV-Test  timeout=${timeout}
    On Access Log Does Not Contain With Offset  from Process ${CLI_SCANNER_PATH}


On Access Doesnt Scan Named Scanner Events
    Register Cleanup   Dump Log On Failure   ${AV_LOG_PATH}
    ${AVPLUGIN_PID} =  Record AV Plugin PID

    ${filepath} =  Set Variable  ${NORMAL_DIRECTORY}/eicar.com

    Mark On Access Log
    Mark AV Log

    Run Scheduled Scan With On Access Enabled
    Wait Until AV Plugin Log Contains With Offset    Completed scan   timeout=60

    On Access Log Does Not Contain With Offset  from Process ${PLUGIN_BINARY}(PID=${AVPLUGIN_PID})
    On Access Log Does Not Contain With Offset  from Process ${SCHEDULED_FILE_WALKER_LAUNCHER}
    On Access Log Does Not Contain With Offset  from Process ${CLI_SCANNER_PATH}


On Access Doesnt Scan On Access Events
    Mark On Access Log
    On-access Scan Eicar Open

    On Access Log Does Not Contain With Offset  from Process ${ON_ACCESS_BIN}


On Access Caches Open Events Without Detections
    ${cleanfile} =  Set Variable  /tmp_test/cleanfile.txt
    ${dirtyfile} =  Set Variable  /tmp_test/dirtyfile.txt

    Create File   ${cleanfile}   ${CLEAN_STRING}
    Register Cleanup   Remove File   ${cleanfile}

    Get File   ${cleanfile}
    Wait Until On Access Log Contains With Offset   Caching ${cleanfile} (Open)

    #On a busy system there maybe a delay between logging we have cached and it being processed in kernel space
    Sleep    1s

    Mark On Access Log
    Get File   ${cleanfile}

    #Generate another event we can expect in logs
    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup   Remove File   ${dirtyfile}
    Wait Until On Access Log Contains With Offset  On-open event for ${dirtyfile} from    timeout=${timeout}
    On Access Log Does Not Contain With Offset   On-open event for ${cleanfile} from


On Access Doesnt Cache Open Events With Detections
    ${dirtyfile} =  Set Variable  /tmp_test/dirtyfile.txt

    Mark On Access Log
    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup   Remove File   ${dirtyfile}
    Wait Until On Access Log Contains With Offset   On-close event for ${dirtyfile} from

    #TODO this is necessary while LINXUDAR-5740 is not resulved
    Append To File  ${dirtyfile}  ${EICAR_STRING}

    Mark On Access Log
    Get File   ${dirtyfile}
    Wait Until On Access Log Contains With Offset  On-open event for ${dirtyfile} from
    Wait Until On Access Log Contains With Offset  Detected "${dirtyfile}" is infected with EICAR-AV-Test (Open)   timeout=${timeout}

    Mark On Access Log
    Get File   ${dirtyfile}
    Wait Until On Access Log Contains With Offset  On-open event for ${dirtyfile} from
    Wait Until On Access Log Contains With Offset  Detected "${dirtyfile}" is infected with EICAR-AV-Test (Open)   timeout=${timeout}


On Access Does Cache Close Events Without Detections
    ${cleanfile} =  Set Variable  /tmp_test/cleanfile.txt
    ${dirtyfile} =  Set Variable  /tmp_test/dirtyfile.txt

    Mark On Access Log
    Create File  ${cleanfile}  ${CLEAN_STRING}
    Register Cleanup   Remove File   ${cleanfile}

    Wait Until On Access Log Contains With Offset  On-close event for ${cleanfile}
    Wait Until On Access Log Contains With Offset  Caching ${cleanfile} (Close-Write)

    #On a busy system there maybe a delay between logging we have cached and it being processed in kernel space
    Sleep    1s

    Mark On Access Log
    Get File   ${cleanfile}

     #Generate another event we can expect in logs
    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup   Remove File   ${dirtyfile}
    Wait Until On Access Log Contains With Offset  Detected "${dirtyfile}" is infected with EICAR-AV-Test (Close-Write)   timeout=${timeout}

    On Access Log Does Not Contain With Offset   On-open event for ${cleanfile} from


On Access Receives Close Event On Cached File
    ${cleanfile} =  Set Variable  /tmp_test/cleanfile.txt
    ${dirtyfile} =  Set Variable  /tmp_test/dirtyfile.txt

    Mark On Access Log
    Create File  ${cleanfile}  ${CLEAN_STRING}
    Register Cleanup   Remove File   ${cleanfile}

    Wait Until On Access Log Contains With Offset  On-close event for ${cleanfile}
    Wait Until On Access Log Contains With Offset  Caching ${cleanfile} (Close-Write)

    #On a busy system there maybe a delay between logging we have cached and it being processed in kernel space
    Sleep    1s

    #Check we are cached
    Mark On Access Log
    Get File   ${cleanfile}

     #Generate another event we can expect in logs
    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup   Remove File   ${dirtyfile}
    Wait Until On Access Log Contains With Offset  Detected "${dirtyfile}" is infected with EICAR-AV-Test (Close-Write)   timeout=${timeout}

    On Access Log Does Not Contain With Offset   On-open event for ${cleanfile} from

    #Check we get an event when we create a close event
    Mark On Access Log
    Append To File   ${cleanfile}   ${CLEAN_STRING}
    Wait Until On Access Log Contains With Offset  On-close event for ${cleanfile}


On Access Doesnt Cache Close Events With Detections
    ${testfile} =  Set Variable  /tmp_test/dirtyfile.txt

    Mark On Access Log
    Create File  ${testfile}  ${EICAR_STRING}
    Register Cleanup   Remove File   ${testfile}

    Wait Until On Access Log Contains With Offset  On-close event for ${testfile} from
    Wait Until On Access Log Contains With Offset  Detected "${testfile}" is infected with EICAR-AV-Test (Close-Write)
    Sleep   1s  #Let the event (hopefully not) be cached

    Mark On Access Log
    Get File  ${testfile}

    Wait Until On Access Log Contains With Offset  On-open event for ${testfile} from
    Wait Until On Access Log Contains With Offset  Detected "${testfile}" is infected with EICAR-AV-Test (Open)


On Access Processes New File With Same Attributes And Contents As Old File
    ${cleanfile} =  Set Variable  /tmp_test/cleanfile.txt

    Mark On Access Log
    Create File   ${cleanfile}   ${CLEAN_STRING}
    Register Cleanup   Remove File   ${cleanfile}

    Get File   ${cleanfile}
    Wait Until On Access Log Contains With Offset  On-open event for ${cleanfile} from    timeout=${timeout}
    Wait Until On Access Log Contains With Offset   Caching ${cleanfile}
    #On a busy system there maybe a delay between logging we have cached and it being processed in kernel space
    Sleep   1

    Remove File   ${cleanfile}

    Mark On Access Log
    Create File   ${cleanfile}   ${CLEAN_STRING}
    Wait Until On Access Log Contains With Offset  On-open event for ${cleanfile} from    timeout=${timeout}


On Access Detects A Clean File Replaced By Dirty File With Same Attributes
    ${dustyfile} =  Set Variable  /tmp_test/notcleanforlongfile.txt
    Mark On Access Log
    Create File   ${dustyfile}   ${CLEAN_STRING}
    Register Cleanup   Remove File   ${dustyfile}
    Sleep   0.1s

    Get File   ${dustyfile}
    Wait Until On Access Log Contains With Offset  On-open event for ${dustyfile} from    timeout=${timeout}
    Wait Until On Access Log Contains With Offset   Caching ${dustyfile}
    #On a busy system there maybe a delay between logging we have cached and it being processed in kernel space
    Sleep   1s

    Remove File   ${dustyfile}
    Create File   ${dustyfile}   ${EICAR_STRING}
    Sleep   0.1s

    Mark On Access Log
    Get File   ${dustyfile}
    Wait Until On Access Log Contains With Offset  On-open event for ${dustyfile} from    timeout=${timeout}
    Wait Until On Access Log Contains With Offset  Detected "${dustyfile}" is infected with EICAR-AV-Test (Open)   timeout=${timeout}
