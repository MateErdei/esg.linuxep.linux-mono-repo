*** Settings ***
Documentation   Product tests for AVP
Force Tags      PRODUCT  AV_BASIC
Library         Collections
Library         DateTime
Library         Process
Library         OperatingSystem
Library         ../Libs/CoreDumps.py
Library         ../Libs/FakeManagement.py
Library         ../Libs/FileSampleObfuscator.py
Library         ../Libs/FileUtils.py
Library         ../Libs/JsonUtils.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/SystemFileWatcher.py
Library         ../Libs/serialisationtools/CapnpHelper.py

Resource    ../shared/ErrorMarkers.robot
Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/BaseResources.robot
Resource    ../shared/FakeManagementResources.robot
Resource    ../shared/SambaResources.robot

Suite Setup    AVBasic Suite Setup
Suite Teardown  AVBasic Suite Teardown

Test Setup     Product Test Setup
Test Teardown  Product Test Teardown

*** Variables ***
${AV_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${AV_LOG_PATH}    ${AV_PLUGIN_PATH}/log/av.log
${HANDLE}


*** Test Cases ***

AV Plugin Will Fail Scan Now If No Policy
    # Test must be before we apply any policies!
    Register Cleanup  Remove File  ${MCS_ACTION_DIRECTORY}/ScanNow_Action*
    Register Cleanup  Exclude Scan As Invalid

    ${mark} =  get_av_log_mark

    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Action  av  ${SAV_APPID}  corr123  ${actionContent}
    wait_for_av_log_contains_after_mark  Refusing to run invalid scan: INVALID  mark=${mark}
    check_av_log_contains_after_mark  Received new Action  mark=${mark}
    check_av_log_contains_after_mark  Evaluating Scan Now  mark=${mark}

AV Plugin Gets Sxl Lookup Setting From SAV Policy
    ${susiStartupSettingsChrootFile} =   Set Variable   ${AV_PLUGIN_PATH}/chroot${SUSI_STARTUP_SETTINGS_FILE}
    Remove Files   ${SUSI_STARTUP_SETTINGS_FILE}   ${susiStartupSettingsChrootFile}

    ${av_mark} =  Get AV Log Mark
    ${policyContent} =   Get SAV Policy   sxlLookupEnabled=false
    send av policy  ${SAV_APPID}  ${policyContent}
    Wait Until Scheduled Scan Updated After Mark  ${av_mark}

    wait_for_log_contains_after_last_restart  ${AV_LOG_PATH}  SAV policy received for the first time.
    Wait Until Created   ${SUSI_STARTUP_SETTINGS_FILE}   timeout=5sec
    ${json} =   load_json_from_file  ${SUSI_STARTUP_SETTINGS_FILE}
    check_json_contains  ${json}  enableSxlLookup  ${false}

AV Plugin Gets ML Lookup Setting From CORE Policy
    ${susiStartupSettingsChrootFile} =   Set Variable   ${AV_PLUGIN_PATH}/chroot${SUSI_STARTUP_SETTINGS_FILE}
    Remove Files   ${SUSI_STARTUP_SETTINGS_FILE}   ${susiStartupSettingsChrootFile}

    send av policy from file  CORE  ${RESOURCES_PATH}/core_policy/CORE-36_ml_disabled.xml

    Wait Until Created   ${SUSI_STARTUP_SETTINGS_FILE}   timeout=5sec
    ${json} =   load_json_from_file  ${SUSI_STARTUP_SETTINGS_FILE}
    check_json_contains  ${json}  machineLearning  ${false}

AV Plugin Can Receive Actions
    ${av_mark} =  Get AV Log Mark
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="Test" id="" subtype="TestAction" replyRequired="1"/>
    Send Plugin Action  av  ${SAV_APPID}  corr123  ${actionContent}
    Wait For AV Log Contains After Mark  Received new Action  ${av_mark}


AV plugin Can Send Status
    [Tags]    PRODUCT  AV_BASIC_STATUS
    ${version} =  Get Version Number From Ini File  ${COMPONENT_ROOT_PATH}/VERSION.ini

    ${status}=  Get Plugin Status  av  SAV
    Should Contain  ${status}   RevID=""
    Should Contain  ${status}   Res="NoRef"
    Should Contain  ${status}   <product-version>${version}</product-version>

    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="123" policyType="2"/></config>
    send av policy  ${SAV_APPID}  ${policyContent}

    Wait For Plugin Status  av  SAV  RevID="123"  Res="Same"  <product-version>${version}</product-version>


AV Plugin Can Process Scan Now
    ${av_mark} =  Get AV Log Mark
    ${exclusions} =  Configure Scan Exclusions Everything Else  /tmp_test/
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/><onDemandScan><posixExclusions><filePathSet>${exclusions}</filePathSet></posixExclusions></onDemandScan></config>
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    send av policy  ${SAV_APPID}  ${policyContent}
    Send Plugin Action  av  ${SAV_APPID}  corr123  ${actionContent}
    Wait For AV Log Contains After Mark  Completed scan Scan Now  ${av_mark}  timeout=180
    Check AV Log Contains After Mark  Received new Action  ${av_mark}
    Check AV Log Contains After Mark  Evaluating Scan Now  ${av_mark}
    Check AV Log Contains After Mark  Starting scan Scan Now  ${av_mark}
    Check ScanNow Log Exists


Scan Now Configuration Is Correct
    Use Fake AVScanner
    Run Scan Now Scan
    Check Scan Now Configuration File is Correct


Scheduled Scan Configuration Is Correct
    Use Fake AVScanner
    Run Scheduled Scan
    Check Scheduled Scan Configuration File is Correct


Scan Now Excludes Files And Directories As Expected
    ${av_mark} =  Get AV Log Mark
    Create Directory  /directory_excluded/
    Create Directory  /file_excluded/

    Create File  /eicar.com                     ${EICAR_STRING}
    Create File  /directory_excluded/eicar.com  ${EICAR_STRING}
    Create File  /file_excluded/eicar.com       ${EICAR_STRING}
    Register Cleanup  Remove Directory  /file_excluded  recursive=True
    Register Cleanup  Remove Directory  /directory_excluded  recursive=True
    Register Cleanup  Remove Files  /eicar.com  /directory_excluded/eicar.com  /file_excluded/eicar.com

    ${scan_now_mark} =  Get Scan Now Log Mark
    Run Scan Now Scan For Excluded Files Test

    Wait For AV Log Contains After Mark  Completed scan Scan Now  ${av_mark}  timeout=240

    Check Scan Now Log Contains After Mark  Excluding file: /eicar.com  ${scan_now_mark}
    Check Scan Now Log Contains After Mark  "/file_excluded/eicar.com" is infected with EICAR-AV-Test  ${scan_now_mark}
    Check Scan Now Log Contains After Mark  Excluding directory: /directory_excluded/  ${scan_now_mark}
    Check Scan Now Log Does Not Contain After Mark  "/directory_excluded/eicar.com" is infected with EICAR-AV-Test  ${scan_now_mark}
    Check Scan Now Log Does Not Contain After Mark  Excluding file: /directory_excluded/eicar.com  ${scan_now_mark}


Scan Now Logs Should Be As Expected
    ${av_mark} =  Get AV Log Mark
    Create Directory  /file_excluded/

    Create File  /file_excluded/eicar.com       ${EICAR_STRING}
    Register Cleanup  Remove Files  /file_excluded/eicar.com
    Register Cleanup  Remove Directory  /file_excluded  recursive=True

    Run Scan Now Scan For Excluded Files Test

    Check AV Log Contains After Mark  Received new Action  ${av_mark}
    Wait For AV Log Contains After Mark  Evaluating Scan Now  ${av_mark}
    Wait For AV Log Contains After Mark  Starting scan Scan Now  ${av_mark}  timeout=10

    Wait For AV Log Contains After Mark  Completed scan Scan Now  ${av_mark}  timeout=240

    File Log Contains             ${SCANNOW_LOG_PATH}        End of Scan Summary:
    File Log Contains             ${SCANNOW_LOG_PATH}        1 file out of
    File Log Contains             ${SCANNOW_LOG_PATH}        1 EICAR-AV-Test infection discovered.
    Check AV Log Does Not Contain After Mark  Notify trimmed output  ${av_mark}



AV Plugin Scans local secondary mount only once
    ${source} =       Set Variable  /tmp_test/ext2.fs
    ${destination} =  Set Variable  /mnt/ext2mnt
    Create Directory  ${destination}
    Create Directory  /tmp_test/
    Create ext2 mount   ${source}   ${destination}
    Register Cleanup  Remove ext2 mount   ${source}   ${destination}
    Create File       ${destination}/eicar.com    ${EICAR_STRING}

    ${scanName} =   Set Variable  testScan
    ${scanName_log} =   Set Variable  ${AV_PLUGIN_PATH}/log/${scanName}.log

    ${allButTmp} =  Configure Scan Exclusions Everything Else  /mnt/
    ${exclusions} =  Set Variable  <posixExclusions><filePathSet>${allButTmp}</filePathSet></posixExclusions>

    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  15 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule>${POLICY_7DAYS}<timeSet><time>${scanTime}</time></timeSet></schedule>
    ${scanObjectSet} =  Policy Fragment FS Types  hardDrives=true
    ${scanSet} =  Set Variable  <onDemandScan>${exclusions}<scanSet><scan><name>${scanName}</name>${schedule}<settings>${scanObjectSet}</settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>

    ${av_mark} =  get_av_log_mark
    send av policy  ${SAV_APPID}  ${policyContent}
    Wait until scheduled scan updated After Mark  ${av_mark}

    wait_for_log_contains_from_mark  ${av_mark}  Scheduled Scan: ${scanName}   timeout=30
    wait_for_log_contains_from_mark  ${av_mark}  Starting scan ${scanName}     timeout=90
    wait_for_log_contains_from_mark  ${av_mark}  Completed scan ${scanName}    timeout=60
    File Should Exist  ${scanName_log}
    File Log Contains  ${scanName_log}  "${destination}/eicar.com" is infected with EICAR-AV-Test (Scheduled)

    ${content} =  Get File   ${scanName_log}  encoding_errors=replace
    ${lines} =  Get Lines Containing String     ${content}  "${destination}/eicar.com" is infected with EICAR
    ${count} =  Get Line Count   ${lines}
    Should Be Equal As Integers  ${count}  ${1}


AV Plugin Can Disable Scanning Of Mounted NFS Shares
    [Tags]  NFS
    ${source} =       Set Variable  /tmp_test/nfsshare
    ${destination} =  Set Variable  /testmnt/nfsshare
    Create Directory  ${source}
    Create File       ${source}/eicar.com    ${EICAR_STRING}
    Register Cleanup  Remove File      ${source}/eicar.com
    Create Directory  ${destination}
    Create Local NFS Share   ${source}   ${destination}

    Test Remote Share  ${destination}


AV Plugin Can Disable Scanning Of Mounted SMB Shares
    [Timeout]  10 minutes
    [Tags]  cifs
    Start Samba
    Register On Fail   Dump Log  /var/log/samba/log.smbd
    Register On Fail   Dump Log  /var/log/samba/log.nmbd
    ${source} =       Set Variable  /tmp_test/smbshare
    ${destination} =  Set Variable  /testmnt/smbshare
    Create Directory  ${source}
    Create File       ${source}/eicar.com    ${EICAR_STRING}
    Register Cleanup  Remove File      ${source}/eicar.com
    Create Directory  ${destination}
    Create Local SMB Share   ${source}   ${destination}

    Test Remote Share  ${destination}


AV Plugin Can Exclude Filepaths From Scheduled Scans
    ${eicar_path1} =  Set Variable  /tmp_test/eicar.com
    ${eicar_path2} =  Set Variable  /tmp_test/eicar.1
    ${eicar_path3} =  Set Variable  /tmp_test/eicar.txt
    ${eicar_path4} =  Set Variable  /tmp_test/eicarStr
    ${eicar_path5} =  Set Variable  /tmp_test/eicar
    Create File      ${eicar_path1}    ${EICAR_STRING}
    Create File      ${eicar_path2}    ${EICAR_STRING}
    Create File      ${eicar_path3}    ${EICAR_STRING}
    Create File      ${eicar_path4}    ${EICAR_STRING}
    Create File      ${eicar_path5}    ${EICAR_STRING}
    ${myscan_log} =   Set Variable  ${AV_PLUGIN_PATH}/log/MyScan.log

    run_on_failure  dump_scheduled_scan_log
    # Remove the scan log so later tests don't fail due to this step
    register late cleanup  Remove File  ${myscan_log}

    ${av_mark} =  Get AV Log Mark
    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  60 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule><daySet><day>monday</day><day>tuesday</day><day>wednesday</day><day>thursday</day><day>friday</day><day>saturday</day><day>sunday</day></daySet><timeSet><time>${scanTime}</time></timeSet></schedule>
    ${allButTmp} =  Configure Scan Exclusions Everything Else  /tmp_test/
    ${exclusions} =  Set Variable  <posixExclusions><filePathSet>${allButTmp}<filePath>${eicar_path1}</filePath><filePath>/tmp_test/eicar.?</filePath><filePath>/tmp_test/*.txt</filePath><filePath>eicarStr</filePath></filePathSet></posixExclusions>
    ${scanSet} =  Set Variable  <onDemandScan>${exclusions}<scanSet><scan><name>MyScan</name>${schedule}<settings><scanObjectSet><CDDVDDrives>false</CDDVDDrives><hardDrives>true</hardDrives><networkDrives>false</networkDrives><removableDrives>false</removableDrives></scanObjectSet></settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>
    send av policy  ${SAV_APPID}  ${policyContent}
    ${myscan_mark} =  Mark Log Size  ${myscan_log}
    Wait until scheduled scan updated After Mark  ${av_mark}

    Wait For AV Log Contains After Mark  Completed scan MyScan  ${av_mark}  timeout=240
    AV Plugin Log Contains  Starting scan MyScan

    # Thread Detector should still be running:
    Check Sophos Threat Detector Running

    File Should Exist  ${myscan_log}
    Check Log Does Not Contain After Mark  ${myscan_log}  "${eicar_path1}" is infected with EICAR-AV-Test  ${myscan_mark}
    Check Log Does Not Contain After Mark  ${myscan_log}  "${eicar_path2}" is infected with EICAR-AV-Test  ${myscan_mark}
    Check Log Does Not Contain After Mark  ${myscan_log}  "${eicar_path3}" is infected with EICAR-AV-Test  ${myscan_mark}
    Check Log Does Not Contain After Mark  ${myscan_log}  "${eicar_path4}" is infected with EICAR-AV-Test  ${myscan_mark}
    Check Log Contains After Mark          ${myscan_log}  "${eicar_path5}" is infected with EICAR-AV-Test (Scheduled)  ${myscan_mark}


AV Plugin Scan Now Does Not Detect PUA
    ${av_mark} =  Get AV Log Mark
    ${scan_now_mark} =  Get Scan Now Log Mark
    Create File      /tmp_test/eicar_pua.com    ${EICAR_PUA_STRING}

    Run Scan Now Scan

    Wait For AV Log Contains After Mark  Completed scan Scan Now  ${av_mark}  timeout=240

    Check AV Log Does Not Contain After Mark  /tmp_test/eicar_pua.com  ${av_mark}

    Check Scan Now Log Does Not Contain After Mark  "/tmp_test/eicar_pua.com" is infected  ${scan_now_mark}

    Exclude Scan Now mount point does not exist

AV Plugin Scan Now with Bind Mount
    ${source} =       Set Variable  /tmp_test/directory
    ${destination} =  Set Variable  /tmp_test/bind_mount
    Register Cleanup  Remove Directory   ${destination}
    Register Cleanup  Remove Directory   ${source}   recursive=true
    Create Directory  ${source}
    Create Directory  ${destination}
    Create File       ${source}/eicar.com    ${EICAR_STRING}

    Run Shell Process   mount --bind ${source} ${destination}     OnError=Failed to create bind mount
    Register Cleanup  Unmount Test Mount  ${destination}

    Should Exist      ${destination}/eicar.com

    ${av_mark} =  Get AV Log Mark
    Run Scan Now Scan
    Wait For AV Log Contains After Mark   Completed scan Scan Now  ${av_mark}  timeout=240

    AV Log Contains Multiple Times After Mark  Found 'EICAR-AV-Test'  ${av_mark}  ${1}


AV Plugin Scan Now with ISO mount
    ${source} =       Set Variable  /tmp_test/eicar.iso
    ${destination} =  Set Variable  /tmp_test/iso_mount
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/eicar.iso  ${source}
    Register Cleanup  Remove File   ${source}
    Create Directory  ${destination}
    Register Cleanup  Remove Directory   ${destination}
    Run Shell Process   mount -o ro,loop ${source} ${destination}     OnError=Failed to create loopback mount
    Register Cleanup  Run Shell Process   umount ${destination}   OnError=Failed to release loopback mount
    Should Exist      ${destination}/DIR/subdir/eicar.com

    ${av_mark} =  Get AV Log Mark
    Run Scan Now Scan
    Wait For AV Log Contains After Mark  Completed scan Scan Now   ${av_mark}  timeout=240
    Check AV Log Contains After Mark  Found 'EICAR-AV-Test' in '${destination}/DIR/subdir/eicar.com'   ${av_mark}

AV Plugin Scan two mounts same inode numbers
    # Mount two copies of the same iso file. inode numbers on the mounts will be identical, but device numbers should
    # differ. We should walk both mounts.
    ${source} =       Set Variable  /tmp_test/eicar.iso
    ${destination} =  Set Variable  /tmp_test/iso_mount
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/eicar.iso  ${source}
    Register Cleanup  Remove File   ${source}
    Create Directory  ${destination}
    Register Cleanup  Remove Directory   ${destination}
    Run Shell Process   mount -o ro,loop ${source} ${destination}     OnError=Failed to create loopback mount
    Register Cleanup  Run Shell Process   umount ${destination}   OnError=Failed to release loopback mount
    Should Exist      ${destination}/DIR/subdir/eicar.com

    ${source2} =       Set Variable  /tmp_test/eicar2.iso
    ${destination2} =  Set Variable  /tmp_test/iso_mount2
    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/eicar.iso  ${source2}
    Register Cleanup  Remove File   ${source2}
    Create Directory  ${destination2}
    Register Cleanup  Remove Directory   ${destination2}
    Run Shell Process   mount -o ro,loop ${source2} ${destination2}     OnError=Failed to create loopback mount
    Register Cleanup  Run Shell Process   umount ${destination2}   OnError=Failed to release loopback mount
    Should Exist      ${destination2}/DIR/subdir/eicar.com

    ${av_mark} =  Get AV Log Mark
    Run Scan Now Scan
    Wait For AV Log Contains After Mark  Completed scan Scan Now   ${av_mark}  timeout=240
    Check AV Log Contains After Mark   Found 'EICAR-AV-Test' in '/tmp_test/iso_mount/DIR/subdir/eicar.com'  ${av_mark}
    Check AV Log Contains After Mark   Found 'EICAR-AV-Test' in '/tmp_test/iso_mount2/DIR/subdir/eicar.com'  ${av_mark}


AV Plugin Gets Customer ID
    ${customerIdFile1} =   Set Variable   ${AV_PLUGIN_PATH}/var/customer_id.txt
    ${customerIdFile2} =   Set Variable   ${AV_PLUGIN_PATH}/chroot${customerIdFile1}

    #Wait for default policy to be processed first
    Wait Until Created   ${customerIdFile1}   timeout=10sec
    Wait Until Created   ${customerIdFile2}   timeout=5sec
    Remove Files   ${customerIdFile1}   ${customerIdFile2}

    ${policyContent} =   Get ALC Policy   userpassword=A  username=B
    # Send av policy logs the policy
    send av policy  ${ALC_APPID}  ${policyContent}

    ${expectedId} =   Set Variable   a1c0f318e58aad6bf90d07cabda54b7d

    Wait Until Created   ${customerIdFile1}   timeout=10sec
    ${customerId1} =   Get File   ${customerIdFile1}
    Should Be Equal   ${customerId1}   ${expectedId}

    Wait Until Created   ${customerIdFile2}   timeout=5sec
    ${customerId2} =   Get File   ${customerIdFile2}
    Should Be Equal   ${customerId2}   ${expectedId}


AV Plugin Gets Customer ID from Obfuscated Creds
    ${customerIdFile1} =   Set Variable   ${AV_PLUGIN_PATH}/var/customer_id.txt
    ${customerIdFile2} =   Set Variable   ${AV_PLUGIN_PATH}/chroot${customerIdFile1}

    #Wait for default policy to be processed first
    Wait Until Created   ${customerIdFile1}   timeout=10sec
    Wait Until Created   ${customerIdFile2}   timeout=5sec
    Remove Files   ${customerIdFile1}   ${customerIdFile2}

    ${policyContent} =   Get ALC Policy
    ...   algorithm=AES256
    ...   userpassword=CCD8CFFX8bdCDFtU0+hv6MvL3FoxA0YeSNjJyZJWxz1b3uTsBu5p8GJqsfp3SAByOZw=
    ...   username=ABC123
    send av policy  ${ALC_APPID}  ${policyContent}

    # md5(md5("ABC123:password"))
    ${expectedId} =   Set Variable   f5c33e370714d94e1d967e53ac4f0437

    Wait Until Created   ${customerIdFile1}   timeout=10sec
    ${customerId1} =   Get File   ${customerIdFile1}
    Should Be Equal   ${customerId1}   ${expectedId}

    Wait Until Created   ${customerIdFile2}   timeout=5sec
    ${customerId2} =   Get File   ${customerIdFile2}
    Should Be Equal   ${customerId2}   ${expectedId}


AV Plugin requests policies at startup
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  FakeManagement Log Contains   Received policy request: APPID=SAV

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  FakeManagement Log Contains   Received policy request: APPID=ALC

AV Plugin Scan Now Can Scan Special File That Cannot Be Read
    Register Cleanup    Exclude SUSI Illegal seek error
    Register Cleanup    Exclude Failed To Scan Special File That Cannot Be Read
    Register Cleanup    Run Process  ip  netns  delete  avtest  stderr=STDOUT
    ${result} =  Run Process  ip  netns  add  avtest  stderr=STDOUT
    Log  output is ${result.stdout}
    Should Be equal As Integers  ${result.rc}  ${0}
    Wait Until File exists  /run/netns/avtest
    ${exclusions} =  Configure Scan Exclusions Everything Else  /run/netns/
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/><onDemandScan><posixExclusions><filePathSet>${exclusions}</filePathSet></posixExclusions></onDemandScan></config>
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    send av policy  ${SAV_APPID}  ${policyContent}

    ${av_mark} =  Get AV Log Mark
    Send Plugin Action  av  ${SAV_APPID}  corr123  ${actionContent}
    Wait For AV Log Contains After Mark  Completed scan Scan Now  ${av_mark}  timeout=180
    Check AV Log Contains After Mark  Received new Action  ${av_mark}
    Check AV Log Contains After Mark  Evaluating Scan Now  ${av_mark}
    Check AV Log Contains After Mark  Starting scan Scan Now  ${av_mark}
    Check ScanNow Log Exists
    File Log Contains  ${SCANNOW_LOG_PATH}  Failed to scan /run/netns/avtest as it could not be read


AV Plugin Can Process SafeStore Flag Enabled
    Start Fake Management If Required
    ${mark} =  get_av_log_mark
    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_safestore_enabled.json
    send av policy  FLAGS  ${policyContent}
    wait for av log contains after mark
    ...   SafeStore flag set. Setting SafeStore to enabled.
    ...   timeout=60
    ...   mark=${mark}


AV Plugin Can Process SafeStore Flag Disabled
    Start Fake Management If Required
    ${mark} =  get_av_log_mark
    send av policy from file  FLAGS  ${RESOURCES_PATH}/flags_policy/flags.json
    wait for av log contains after mark
    ...   SafeStore flag not set. Setting SafeStore to disabled.
    ...   timeout=60
    ...   mark=${mark}

AV Plugin Replaces Path With Request To Check Log If Path Contains Bad Unicode
    Create Bad Unicode Eicars

    ${avmark} =  LogUtils.get_av_log_mark

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLI_SCANNER_PATH} /tmp_test/
    Log  return code is ${rc}
    Log  output is ${output}

    av_log_contains_multiple_times_after_mark   See endpoint logs for threat file path at: /opt/sophos-spl/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log   ${avmark}  3

    Dump Log  ${AV_LOG_PATH}


*** Keywords ***
AVBasic Suite Setup
    Start Fake Management If Required
    set_default_policy_from_file  ALC    ${RESOURCES_PATH}/alc_policy/template/base_and_av_VUT.xml
    Create File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector

AVBasic Suite Teardown
    Remove File  ${COMPONENT_ROOT_PATH}/var/inhibit_system_file_change_restart_threat_detector


Clear Logs
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check AV Plugin Not Running

    Log  Backup logs before removing them
    Dump Log  ${AV_LOG_PATH}
    Dump Log  ${THREAT_DETECTOR_LOG_PATH}
    Dump Log  ${SUSI_DEBUG_LOG_PATH}
    Dump Log  ${SAFESTORE_LOG_PATH}

    Remove File    ${AV_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_LOG_PATH}
    Remove File    ${SUSI_DEBUG_LOG_PATH}*
    Remove File    ${SAFESTORE_LOG_PATH}

Product Test Setup
    SystemFileWatcher.Start Watching System Files
    Register Cleanup      SystemFileWatcher.stop watching system files

    Remove File  ${THREAT_DATABASE_PATH}
    Start AV
    Component Test Setup
    Delete Eicars From Tmp
    register on fail  Dump Log  ${COMPONENT_ROOT_PATH}/log/${COMPONENT_NAME}.log
    register on fail  Dump Log  ${FAKEMANAGEMENT_AGENT_LOG_PATH}
    register on fail  Dump Log  ${THREAT_DETECTOR_LOG_PATH}
    Register Cleanup  Check All Product Logs Do Not Contain Error
    Register Cleanup  Exclude CustomerID Failed To Read Error
    Register Cleanup  Require No Unhandled Exception
    Register Cleanup  Check For Coredumps  ${TEST NAME}
    Register Cleanup  Check Dmesg For Segfaults

Product Test Teardown
    Delete Eicars From Tmp
    run teardown functions

    Component Test TearDown
    Remove File  ${SOPHOS_INSTALL}/base/telemetry/cache/av-telemetry.json
    #Run clear logs only after we stopped all the processes
    ${result} =   Check If The Logs Are Close To Rotating
    run keyword if  ${result}   Clear Logs
    Run Keyword If Test Failed  Clear logs

Test Remote Share
    [Arguments]  ${destination}
    Should Exist  ${destination}/eicar.com
    ${remoteFSscanningDisabled} =   Set Variable  remoteFSscanningDisabled
    ${remoteFSscanningEnabled} =    Set Variable  remoteFSscanningEnabled
    ${remoteFSscanningDisabled_log} =   Set Variable  ${AV_PLUGIN_PATH}/log/${remoteFSscanningDisabled}.log
    ${remoteFSscanningEnabled_log} =   Set Variable  ${AV_PLUGIN_PATH}/log/${remoteFSscanningEnabled}.log

    ${allButTmp} =  Configure Scan Exclusions Everything Else  /testmnt/
    ${exclusions} =  Set Variable  <posixExclusions><filePathSet>${allButTmp}</filePathSet></posixExclusions>

    ${av_mark} =  Get AV Log Mark
    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  15 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule>${POLICY_7DAYS}<timeSet><time>${scanTime}</time></timeSet></schedule>
    ${scanObjectSet} =  Policy Fragment FS Types  networkDrives=false
    ${scanSet} =  Set Variable  <onDemandScan>${exclusions}<scanSet><scan><name>${remoteFSscanningDisabled}</name>${schedule}<settings>${scanObjectSet}</settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>
    send av policy  ${SAV_APPID}  ${policyContent}
    Wait Until Scheduled Scan Updated After Mark  ${av_mark}
    Wait For AV Log Contains After Mark  Starting scan ${remoteFSscanningDisabled}  ${av_mark}  timeout=120
    Require Sophos Threat Detector Running
    Wait For AV Log Contains After Mark  Completed scan ${remoteFSscanningDisabled}  ${av_mark}  timeout=240
    File Should Exist  ${remoteFSscanningDisabled_log}
    File Log Should Not Contain  ${remoteFSscanningDisabled_log}  "${destination}/eicar.com" is infected with EICAR

    register on fail  dump log  ${remoteFSscanningEnabled_log}
    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  15 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule>${POLICY_7DAYS}<timeSet><time>${scanTime}</time></timeSet></schedule>
    ${scanObjectSet} =  Policy Fragment FS Types  networkDrives=true
    ${scanSet} =  Set Variable  <onDemandScan>${exclusions}<scanSet><scan><name>${remoteFSscanningEnabled}</name>${schedule}<settings>${scanObjectSet}</settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>
    send av policy  ${SAV_APPID}  ${policyContent}

    Wait For AV Log Contains After Mark  Starting scan ${remoteFSscanningEnabled}  ${av_mark}  timeout=120
    Require Sophos Threat Detector Running
    Wait For AV Log Contains After Mark  Completed scan ${remoteFSscanningEnabled}  ${av_mark}  timeout=240
    File Should Exist  ${remoteFSscanningEnabled_log}
    File Log Contains  ${remoteFSscanningEnabled_log}  "${destination}/eicar.com" is infected with EICAR
