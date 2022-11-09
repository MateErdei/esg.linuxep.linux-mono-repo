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

Mount Image and Wait For Log
    [Arguments]  ${where}  ${image}  ${type}  ${opts}=loop
    ${mark} =  get_on_access_log_mark
    Mount Image  ${where}  ${image}  ${type}  ${opts}
    wait for on access log contains after mark  Including mount point: ${where}  mark=${mark}

On Access Scans Eicar On Filesystem
    [Arguments]  ${type}  ${image}  ${opts}=loop

    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    Mount Image and Wait For Log  ${where}  ${image}  ${type}  ${opts}

    ${pid} =  Get Robot Pid
    ${mark} =  get_on_access_log_mark
    Create File  ${where}/eicar.com  ${EICAR_STRING}
    Register Cleanup  Remove File  ${where}/eicar.com

    wait for on access log contains after mark  On-close event for ${where}/eicar.com from  mark=${mark}
    wait for on access log contains after mark  (PID=${pid}) and UID 0  mark=${mark}
    wait for on access log contains after mark  Detected "${where}/eicar.com" is infected with  mark=${mark}  timeout=${timeout}

On Access Scans Eicar On Filesystem from Image
    [Arguments]  ${type}  ${imageName}  ${opts}=loop

    Require Filesystem  ${type}
    ${image} =  Copy And Extract Image  ${imageName}
    On Access Scans Eicar On Filesystem  ${type}  ${image}

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
    wait for on access log contains after mark   /corrupted.xls as it is corrupted  mark=${mark}  timeout=${timeout}


On Access Scans File On BFS
    On Access Scans Eicar On Filesystem from Image  bfs  bfsFileSystem

On Access Scans File On CRAMFS
    On Access Scans Eicar On Filesystem from Image  cramfs  cramfsFileSystem

On Access Scans File On EXT2
    On Access Scans Eicar On Filesystem from Image  ext2  ext2FileSystem

On Access Scans File On EXT3
    On Access Scans Eicar On Filesystem from Image  ext3  ext3FileSystem

On Access Scans File On EXT4
    On Access Scans Eicar On Filesystem from Image  ext4  ext4FileSystem

On Access Scans File On MINIX
    # Can't require minix fs in /proc/filesystems since it could be dynamically loaded
    Run Keyword And Ignore Error
    ...  Run Process  modprobe  minix
    On Access Scans Eicar On Filesystem from Image  minix  minixFileSystem

On Access Scans File On MSDOS
    # vfat is fs type for msdos
    ${opts} =  Set Variable  loop,umask=0000
    On Access Scans Eicar On Filesystem from Image  vfat  msdosFileSystem  opts=${opts}

On Access Scans File On NTFS
    # Can't check for filesystem, since NTFS uses fuse
    ${file_does_not_exist} =  Does File Not Exist  /sbin/mount.ntfs
    Pass Execution If    ${file_does_not_exist}  /sbin/mount.ntfs doesn't exist - NTFS not supported

    ${type} =  Set Variable  ntfs
    ${image} =  Copy And Extract Image  ntfsFileSystem
    On Access Scans Eicar On Filesystem  ${type}  ${image}

On Access Scans File On ReiserFS
    Run Keyword And Ignore Error
    ...  Run Process  modprobe  reiserfs
    On Access Scans Eicar On Filesystem from Image  reiserfs  reiserfsFileSystem

On Access Scans File On SquashFS
    # Squash is read-only
    ${type} =  Set Variable  squashfs
    Run Keyword And Ignore Error
    ...  Run Process  modprobe  ${type}
    Require Filesystem  ${type}

    ${image} =  Copy And Extract Image  squashfsFileSystem
    ${where} =  Set Variable  ${NORMAL_DIRECTORY}/mount

    ${mark} =  get_on_access_log_mark
    Mount Image and Wait For Log  ${where}  ${image}  ${type}

    ${pid} =  Get Robot Pid
    ${contents} =  Get Binary File  ${NORMAL_DIRECTORY}/mount/eicar.com

    wait for on access log contains after mark  On-open event for ${where}/eicar.com from  mark=${mark}
    wait for on access log contains after mark  (PID=${pid}) and UID 0  mark=${mark}
    wait for on access log contains after mark  Detected "${where}/eicar.com" is infected with   mark=${mark}  timeout=${timeout}


On Access Scans File On VFAT
    ${opts} =  Set Variable  loop,umask=0000
    On Access Scans Eicar On Filesystem from Image  vfat  vfatFileSystem  opts=${opts}

On Access Scans File On XFS
    Run Keyword And Ignore Error
    ...  Run Process  modprobe  xfs
    ${opts} =  Set Variable  nouuid
    On Access Scans Eicar On Filesystem from Image  xfs  xfsFileSystem  opts=${opts}

On Access Includes Included Mount On Top Of Excluded Mount
    ${excludedMount} =  Set Variable  /proc/tty
    ${includedMount} =  Set Variable  ${NORMAL_DIRECTORY}/test
    ${dest} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    Directory Should Exist  ${excludedMount}
    Create Directory  ${includedMount}

    ${mark} =  get_on_access_log_mark
    Bind Mount Directory  ${excludedMount}  ${dest}
    wait for on access log contains after mark
    ...  Mount point ${dest} is system and will be excluded from the scan
    ...  mark=${mark}
    Bind Mount Directory  ${includedMount}  ${dest}
    wait for on access log contains after mark  Including mount point: ${dest}  mark=${mark}


On Access Excludes Excluded Mount On Top Of Included Mount
    ${excludedMount} =  Set Variable  /proc/tty
    ${includedMount} =  Set Variable  ${NORMAL_DIRECTORY}/test
    ${dest} =  Set Variable  ${NORMAL_DIRECTORY}/mount
    Directory Should Exist  ${excludedMount}
    Create Directory  ${includedMount}

    ${mark} =  get_on_access_log_mark
    Bind Mount Directory  ${includedMount}  ${dest}
    wait for on access log contains after mark  Including mount point: ${dest}  mark=${mark}
    Bind Mount Directory  ${excludedMount}  ${dest}
    wait for on access log contains after mark  Mount point ${dest} is system and will be excluded from the scan  mark=${mark}


On Access Doesnt Scan Threat Detector Events
    ${TD_PID} =  Record Sophos Threat Detector PID
    ${filepath} =  Set Variable  ${NORMAL_DIRECTORY}/eicar.com

    ${mark} =  get_on_access_log_mark

    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    wait for on access log contains after mark   Detected "${filepath}" is infected with EICAR-AV-Test  mark=${mark}  timeout=${timeout}
    check_on_access_log_does_not_contain_after_mark   from Process ${SOPHOS_THREAT_DETECTOR_BINARY}(PID=${TD_PID})  mark=${mark}


On Access Doesnt Scan CommandLine Scanner Events
    ${filepath} =  Set Variable  ${NORMAL_DIRECTORY}/eicar.com

    ${mark} =  get_on_access_log_mark

    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} ${NORMAL_DIRECTORY}

    Should Be Equal As Integers  ${rc}  ${VIRUS_DETECTED_RESULT}
    Should Contain   ${output}    Detected "${filepath}" is infected with EICAR-AV-Test

    wait for on access log contains after mark       Detected "${filepath}" is infected with EICAR-AV-Test  mark=${mark}  timeout=${timeout}
    check_on_access_log_does_not_contain_after_mark  from Process ${CLI_SCANNER_PATH}  mark=${mark}


On Access Doesnt Scan Named Scanner Events
    Register Cleanup   Dump Log On Failure   ${AV_LOG_PATH}
    ${AVPLUGIN_PID} =  Record AV Plugin PID

    ${filepath} =  Set Variable  ${NORMAL_DIRECTORY}/eicar.com

    ${oamark} =  get_on_access_log_mark
    ${avmark} =  get_av_log_mark

    Run Scheduled Scan With On Access Enabled
    wait_for_av_log_contains_after_mark   Completed scan   mark=${avmark}  timeout=60

    check_on_access_log_does_not_contain_after_mark  from Process ${PLUGIN_BINARY}(PID=${AVPLUGIN_PID})  mark=${oamark}
    check_on_access_log_does_not_contain_after_mark  from Process ${SCHEDULED_FILE_WALKER_LAUNCHER}  mark=${oamark}
    check_on_access_log_does_not_contain_after_mark  from Process ${CLI_SCANNER_PATH}  mark=${oamark}


On Access Doesnt Scan On Access Events
    ${oamark} =  get_on_access_log_mark
    On-access Scan Eicar Open

    check_on_access_log_does_not_contain_after_mark  from Process ${ON_ACCESS_BIN}  mark=${oamark}


On Access Caches Open Events Without Detections
    ${cleanfile} =  Set Variable  /tmp_test/cleanfile.txt
    ${dirtyfile} =  Set Variable  /tmp_test/dirtyfile.txt

    ${oamark} =  get_on_access_log_mark

    Create File   ${cleanfile}-excluded   ${CLEAN_STRING}
    Evaluate  shutil.move("${cleanfile}-excluded", "${cleanfile}")
    Register Cleanup   Remove File   ${cleanfile}

    Get File   ${cleanfile}
    wait for on access log contains after mark   Caching ${cleanfile} (Open)  mark=${oamark}

    #On a busy system there maybe a delay between logging we have cached and it being processed in kernel space
    Sleep    1s

    ${oamark} =  get_on_access_log_mark
    Get File   ${cleanfile}

    #Generate another event we can expect in logs
    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup   Remove File   ${dirtyfile}
    wait for on access log contains after mark        On-open event for ${dirtyfile} from  mark=${oamark}    timeout=${timeout}
    check_on_access_log_does_not_contain_after_mark   On-open event for ${cleanfile} from  mark=${oamark}
    dump logs


On Access Doesnt Cache Open Events With Detections
    ${dirtyfile} =  Set Variable  /tmp_test/dirtyfile.txt

    ${oamark} =  get_on_access_log_mark
    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup   Remove File   ${dirtyfile}
    wait for on access log contains after mark    On-close event for ${dirtyfile} from  mark=${oamark}

    #TODO this is necessary while LINXUDAR-5740 is not resolved
    Append To File  ${dirtyfile}  ${EICAR_STRING}

    ${oamark} =  get_on_access_log_mark
    Get File   ${dirtyfile}
    wait for on access log contains after mark  On-open event for ${dirtyfile} from  mark=${oamark}
    wait for on access log contains after mark  Detected "${dirtyfile}" is infected with EICAR-AV-Test (Open)  mark=${oamark}   timeout=${timeout}

    ${oamark} =  get_on_access_log_mark
    Get File   ${dirtyfile}
    wait for on access log contains after mark  On-open event for ${dirtyfile} from  mark=${oamark}
    wait for on access log contains after mark  Detected "${dirtyfile}" is infected with EICAR-AV-Test (Open)  mark=${oamark}   timeout=${timeout}


On Access Does Cache Close Events Without Detections
    ${cleanfile} =  Set Variable  /tmp_test/cleanfile.txt
    ${dirtyfile} =  Set Variable  /tmp_test/dirtyfile.txt

    ${oamark} =  get_on_access_log_mark
    Create File  ${cleanfile}  ${CLEAN_STRING}
    Register Cleanup   Remove File   ${cleanfile}

    wait for on access log contains after mark  On-close event for ${cleanfile}  mark=${oamark}
    wait for on access log contains after mark  Caching ${cleanfile} (Close-Write)  mark=${oamark}

    #On a busy system there maybe a delay between logging we have cached and it being processed in kernel space
    Sleep    1s

    ${oamark} =  get_on_access_log_mark
    Get File   ${cleanfile}

     #Generate another event we can expect in logs
    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup   Remove File   ${dirtyfile}
    wait for on access log contains after mark  Detected "${dirtyfile}" is infected with EICAR-AV-Test (Close-Write)  mark=${oamark}   timeout=${timeout}

    check_on_access_log_does_not_contain_after_mark  On-open event for ${cleanfile} from  mark=${oamark}


On Access Receives Close Event On Cached File
    ${cleanfile} =  Set Variable  /tmp_test/cleanfile.txt
    ${dirtyfile} =  Set Variable  /tmp_test/dirtyfile.txt

    ${oamark} =  get_on_access_log_mark
    Create File  ${cleanfile}  ${CLEAN_STRING}
    Register Cleanup   Remove File   ${cleanfile}

    wait for on access log contains after mark  On-close event for ${cleanfile}  mark=${oamark}
    wait for on access log contains after mark  Caching ${cleanfile} (Close-Write)  mark=${oamark}

    #On a busy system there maybe a delay between logging we have cached and it being processed in kernel space
    Sleep    1s

    #Check we are cached
    ${oamark} =  get_on_access_log_mark
    Get File   ${cleanfile}

     #Generate another event we can expect in logs
    Create File  ${dirtyfile}  ${EICAR_STRING}
    Register Cleanup   Remove File   ${dirtyfile}
    wait for on access log contains after mark  Detected "${dirtyfile}" is infected with EICAR-AV-Test (Close-Write)  mark=${oamark}   timeout=${timeout}

    check_on_access_log_does_not_contain_after_mark  On-open event for ${cleanfile} from  mark=${oamark}

    #Check we get an event when we create a close event
    ${oamark} =  get_on_access_log_mark
    Append To File   ${cleanfile}   ${CLEAN_STRING}
    wait for on access log contains after mark  On-close event for ${cleanfile}  mark=${oamark}


On Access Doesnt Cache Close Events With Detections
    ${testfile} =  Set Variable  /tmp_test/dirtyfile.txt

    ${oamark} =  get_on_access_log_mark
    Create File  ${testfile}  ${EICAR_STRING}
    Register Cleanup   Remove File   ${testfile}

    wait for on access log contains after mark  On-close event for ${testfile} from  mark=${oamark}
    wait for on access log contains after mark  Detected "${testfile}" is infected with EICAR-AV-Test (Close-Write)  mark=${oamark}
    Sleep   1s  #Let the event (hopefully not) be cached

    ${oamark} =  get_on_access_log_mark
    Get File  ${testfile}

    wait for on access log contains after mark  On-open event for ${testfile} from  mark=${oamark}
    wait for on access log contains after mark  Detected "${testfile}" is infected with EICAR-AV-Test (Open)  mark=${oamark}


On Access Processes New File With Same Attributes And Contents As Old File
    ${cleanfile} =  Set Variable  /tmp_test/cleanfile.txt

    ${oamark} =  get_on_access_log_mark
    Create File   ${cleanfile}   ${CLEAN_STRING}
    Register Cleanup   Remove File   ${cleanfile}

    Get File   ${cleanfile}
    wait for on access log contains after mark  On-open event for ${cleanfile} from  mark=${oamark}  timeout=${timeout}
    wait for on access log contains after mark  Caching ${cleanfile}  mark=${oamark}
    #On a busy system there maybe a delay between logging we have cached and it being processed in kernel space
    Sleep   1

    Remove File   ${cleanfile}

    ${oamark} =  get_on_access_log_mark
    Create File   ${cleanfile}   ${CLEAN_STRING}
    wait for on access log contains after mark  On-open event for ${cleanfile} from   mark=${oamark}  timeout=${timeout}


On Access Detects A Clean File Replaced By Dirty File With Same Attributes
    ${dustyfile} =  Set Variable  /tmp_test/notcleanforlongfile.txt
    ${oamark} =  get_on_access_log_mark
    Create File   ${dustyfile}   ${CLEAN_STRING}
    Register Cleanup   Remove File   ${dustyfile}
    Sleep   0.1s

    Get File   ${dustyfile}
    wait for on access log contains after mark  On-open event for ${dustyfile} from   mark=${oamark}   timeout=${timeout}
    wait for on access log contains after mark  Caching ${dustyfile}  mark=${oamark}
    #On a busy system there maybe a delay between logging we have cached and it being processed in kernel space
    Sleep   1s

    Remove File   ${dustyfile}
    Create File   ${dustyfile}   ${EICAR_STRING}
    Sleep   0.1s

    ${oamark} =  get_on_access_log_mark
    Get File   ${dustyfile}
    wait for on access log contains after mark  On-open event for ${dustyfile} from    mark=${oamark}  timeout=${timeout}
    wait for on access log contains after mark  Detected "${dustyfile}" is infected with EICAR-AV-Test (Open)   mark=${oamark}  timeout=${timeout}
