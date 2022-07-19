*** Settings ***
Documentation   Product tests for AVP
Force Tags      PRODUCT  AV_BASIC
Library         Collections
Library         DateTime
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py
Library         ../Libs/FileUtils.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/SystemFileWatcher.py
Library         ../Libs/Telemetry.py
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
${TESTSYSFILE}  hosts
${TESTSYSPATHBACKUP}  /etc/${TESTSYSFILE}backup
${TESTSYSPATH}  /etc/${TESTSYSFILE}
${SOMETIMES_SYMLINKED_SYSFILE}  resolv.conf
${SOMETIMES_SYMLINKED_SYSPATHBACKUP}  /etc/${SOMETIMES_SYMLINKED_SYSFILE}backup
${SOMETIMES_SYMLINKED_SYSPATH}  /etc/${SOMETIMES_SYMLINKED_SYSFILE}


*** Test Cases ***
AV Plugin Can Receive Actions
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="Test" id="" subtype="TestAction" replyRequired="1"/>
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains With Offset  Received new Action


AV plugin Can Send Status
    [Tags]    PRODUCT  AV_BASIC_STATUS
    ${version} =  Get Version Number From Ini File  ${COMPONENT_ROOT_PATH}/VERSION.ini

    ${status}=  Get Plugin Status  av  SAV
    Should Contain  ${status}   RevID=""
    Should Contain  ${status}   Res="NoRef"
    Should Contain  ${status}   <product-version>${version}</product-version>

    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="123" policyType="2"/></config>
    Send Plugin Policy  av  sav  ${policyContent}

    Wait For Plugin Status  av  SAV  RevID="123"  Res="Same"  <product-version>${version}</product-version>


AV Plugin Can Process Scan Now
    ${exclusions} =  Configure Scan Exclusions Everything Else  /tmp_test/
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/><onDemandScan><posixExclusions><filePathSet>${exclusions}</filePathSet></posixExclusions></onDemandScan></config>
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Policy  av  sav  ${policyContent}
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now  timeout=180  interval=5
    AV Plugin Log Contains With Offset  Received new Action
    AV Plugin Log Contains With Offset  Evaluating Scan Now
    AV Plugin Log Contains With Offset  Starting scan Scan Now
    Check ScanNow Log Exists


AV Plugin Scan Now Updates Telemetry Count
    # reset telemetry count
    ${telemetryString}=  Get Plugin Telemetry  av
    Log   ${telemetryString}

    ${exclusions} =  Configure Scan Exclusions Everything Else  /tmp_test/
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/><onDemandScan><posixExclusions><filePathSet>${exclusions}</filePathSet></posixExclusions></onDemandScan></config>
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Policy  av  sav  ${policyContent}
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now  timeout=180  interval=5
    AV Plugin Log Contains With Offset  Received new Action
    AV Plugin Log Contains With Offset  Evaluating Scan Now
    AV Plugin Log Contains With Offset  Starting scan Scan Now
    Check ScanNow Log Exists

    ${telemetryString}=  Get Plugin Telemetry  av
    Log   ${telemetryString}
    ${telemetryJson}=    Evaluate     json.loads("""${telemetryString}""")    json

    Dictionary Should Contain Item   ${telemetryJson}   scan-now-count   1

    av_log_contains_only_one_no_saved_telemetry_per_start


Scan Now Configuration Is Correct
    Use Fake AVScanner
    Run Scan Now Scan
    Check Scan Now Configuration File is Correct


Scheduled Scan Configuration Is Correct
    Use Fake AVScanner
    Run Scheduled Scan
    Check Scheduled Scan Configuration File is Correct


Scan Now Excludes Files And Directories As Expected
    Mark AV Log
    Create Directory  /directory_excluded/
    Create Directory  /file_excluded/

    Create File  /eicar.com                     ${EICAR_STRING}
    Create File  /directory_excluded/eicar.com  ${EICAR_STRING}
    Create File  /file_excluded/eicar.com       ${EICAR_STRING}
    Register Cleanup  Remove Directory  /file_excluded  recursive=True
    Register Cleanup  Remove Directory  /directory_excluded  recursive=True
    Register Cleanup  Remove Files  /eicar.com  /directory_excluded/eicar.com  /file_excluded/eicar.com

    Run Scan Now Scan For Excluded Files Test

    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now  timeout=240  interval=5

    File Log Contains             ${SCANNOW_LOG_PATH}        Excluding file: /eicar.com
    File Log Contains             ${SCANNOW_LOG_PATH}        "/file_excluded/eicar.com" is infected with EICAR-AV-Test
    File Log Contains             ${SCANNOW_LOG_PATH}        Excluding directory: /directory_excluded/
    File Log Should Not Contain   ${SCANNOW_LOG_PATH}        "/directory_excluded/eicar.com" is infected with EICAR-AV-Test
    File Log Should Not Contain   ${SCANNOW_LOG_PATH}        Excluding file: /directory_excluded/eicar.com


Scan Now Logs Should Be As Expected
    Create Directory  /file_excluded/

    Create File  /file_excluded/eicar.com       ${EICAR_STRING}
    Register Cleanup  Remove Files  /file_excluded/eicar.com
    Register Cleanup  Remove Directory  /file_excluded  recursive=True

    Run Scan Now Scan For Excluded Files Test

    AV Plugin Log Contains With Offset  Received new Action
    Wait Until AV Plugin Log Contains With Offset  Evaluating Scan Now
    Wait Until AV Plugin Log Contains With Offset  Starting scan Scan Now  timeout=10

    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now  timeout=240  interval=5

    File Log Contains             ${SCANNOW_LOG_PATH}        End of Scan Summary:
    File Log Contains             ${SCANNOW_LOG_PATH}        1 file out of
    File Log Contains             ${SCANNOW_LOG_PATH}        1 EICAR-AV-Test infection discovered.
    AV Plugin Log Should Not Contain With Offset             Notify trimmed output


AV Plugin Will Fail Scan Now If No Policy
    Register Cleanup  Remove File  ${MCS_ACTION_DIRECTORY}/ScanNow_Action*
    Register Cleanup  Exclude Scan As Invalid

    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains With Offset  Refusing to run invalid scan: INVALID
    AV Plugin Log Contains With Offset  Received new Action
    AV Plugin Log Contains With Offset  Evaluating Scan Now


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
    Send Plugin Policy  av  sav  ${policyContent}
    Wait until scheduled scan updated With Offset

    Wait Until AV Plugin Log Contains With Offset  Scheduled Scan: ${scanName}   timeout=30
    Wait Until AV Plugin Log Contains With Offset  Starting scan ${scanName}     timeout=90
    Wait Until AV Plugin Log Contains With Offset  Completed scan ${scanName}    timeout=60
    File Should Exist  ${scanName_log}
    File Log Contains  ${scanName_log}  "${destination}/eicar.com" is infected with EICAR

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
    Register Cleanup  Remove Local NFS Share   ${source}   ${destination}

    Test Remote Share  ${destination}


AV Plugin Can Disable Scanning Of Mounted SMB Shares
    [Timeout]  10 minutes
    [Tags]  SMB
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

    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  60 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule><daySet><day>monday</day><day>tuesday</day><day>wednesday</day><day>thursday</day><day>friday</day><day>saturday</day><day>sunday</day></daySet><timeSet><time>${scanTime}</time></timeSet></schedule>
    ${allButTmp} =  Configure Scan Exclusions Everything Else  /tmp_test/
    ${exclusions} =  Set Variable  <posixExclusions><filePathSet>${allButTmp}<filePath>${eicar_path1}</filePath><filePath>/tmp_test/eicar.?</filePath><filePath>/tmp_test/*.txt</filePath><filePath>eicarStr</filePath></filePathSet></posixExclusions>
    ${scanSet} =  Set Variable  <onDemandScan>${exclusions}<scanSet><scan><name>MyScan</name>${schedule}<settings><scanObjectSet><CDDVDDrives>false</CDDVDDrives><hardDrives>true</hardDrives><networkDrives>false</networkDrives><removableDrives>false</removableDrives></scanObjectSet></settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>
    Send Plugin Policy  av  sav  ${policyContent}
    Wait until scheduled scan updated With Offset

    Wait Until AV Plugin Log Contains  Completed scan MyScan  timeout=240  interval=5
    AV Plugin Log Contains  Starting scan MyScan

    # Thread Detector should still be running:
    Check Sophos Threat Detector Running

    File Should Exist  ${myscan_log}
    File Log Should Not Contain  ${myscan_log}  "${eicar_path1}" is infected with EICAR-AV-Test
    File Log Should Not Contain  ${myscan_log}  "${eicar_path2}" is infected with EICAR-AV-Test
    File Log Should Not Contain  ${myscan_log}  "${eicar_path3}" is infected with EICAR-AV-Test
    File Log Should Not Contain  ${myscan_log}  "${eicar_path4}" is infected with EICAR-AV-Test
    File Log Contains            ${myscan_log}  "${eicar_path5}" is infected with EICAR-AV-Test


AV Plugin Scan of Infected File Increases Threat Eicar Count And Reports Suspicious Threat Health
    Create File      /tmp_test/eicar.com    ${EICAR_STRING}
    Register Cleanup  Remove File  /tmp_test/eicar.com
    Remove Files      /file_excluded/eicar.com  /tmp_test/smbshare/eicar.com

    # Run telemetry to reset counters to 0
    ${telemetryString}=  Get Plugin Telemetry  av
    ${telemetryJson}=    Evaluate     json.loads("""${telemetryString}""")    json
    Dictionary Should Contain Item   ${telemetryJson}   threatHealth   1

    Run Scan Now Scan

    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now  timeout=240  interval=5

    ${telemetryString}=  Get Plugin Telemetry  av
    ${telemetryJson}=    Evaluate     json.loads("""${telemetryString}""")    json
    Log   ${telemetryJson}
    Dictionary Should Contain Item   ${telemetryJson}   threat-eicar-count   1
    Dictionary Should Contain Item   ${telemetryJson}   threatHealth   2


AV Plugin Scan Now Does Not Detect PUA
    Create File      /tmp_test/eicar_pua.com    ${EICAR_PUA_STRING}

    Run Scan Now Scan

    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now  timeout=240  interval=5

    AV Plugin Log Does Not Contain With Offset  /tmp_test/eicar_pua.com

    File Log Should Not Contain With Offset  ${SCANNOW_LOG_PATH}  "/tmp_test/eicar_pua.com" is infected

AV Plugin Scan Now with Bind Mount
    ${source} =       Set Variable  /tmp_test/directory
    ${destination} =  Set Variable  /tmp_test/bind_mount
    Register Cleanup  Remove Directory   ${destination}
    Register Cleanup  Remove Directory   ${source}   recursive=true
    Create Directory  ${source}
    Create Directory  ${destination}
    Create File       ${source}/eicar.com    ${EICAR_STRING}

    Run Shell Process   mount --bind ${source} ${destination}     OnError=Failed to create bind mount
    Register Cleanup  Run Shell Process   umount ${destination}   OnError=Failed to release bind mount

    Should Exist      ${destination}/eicar.com

    Run Scan Now Scan
    Wait Until AV Plugin Log Contains With Offset   Completed scan Scan Now   timeout=240   interval=5

    ${count} =        Count Lines In Log With Offset   ${AV_LOG_PATH}  Found 'EICAR-AV-Test'  ${AV_LOG_MARK}
    Should Be Equal As Integers  ${1}  ${count}


AV Plugin Scan Now with ISO mount
    ${source} =       Set Variable  /tmp_test/eicar.iso
    ${destination} =  Set Variable  /tmp_test/iso_mount
    Copy File  ${RESOURCES_PATH}/file_samples/eicar.iso  ${source}
    Register Cleanup  Remove File   ${source}
    Create Directory  ${destination}
    Register Cleanup  Remove Directory   ${destination}
    Run Shell Process   mount -o ro,loop ${source} ${destination}     OnError=Failed to create loopback mount
    Register Cleanup  Run Shell Process   umount ${destination}   OnError=Failed to release loopback mount
    Should Exist      ${destination}/directory/subdir/eicar.com

    Run Scan Now Scan
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now   timeout=240   interval=5
    AV Plugin Log Contains With Offset  Found 'EICAR-AV-Test' in '/tmp_test/iso_mount/directory/subdir/eicar.com'


AV Plugin Scan two mounts same inode numbers
    # Mount two copies of the same iso file. inode numbers on the mounts will be identical, but device numbers should
    # differ. We should walk both mounts.
    ${source} =       Set Variable  /tmp_test/eicar.iso
    ${destination} =  Set Variable  /tmp_test/iso_mount
    Copy File  ${RESOURCES_PATH}/file_samples/eicar.iso  ${source}
    Register Cleanup  Remove File   ${source}
    Create Directory  ${destination}
    Register Cleanup  Remove Directory   ${destination}
    Run Shell Process   mount -o ro,loop ${source} ${destination}     OnError=Failed to create loopback mount
    Register Cleanup  Run Shell Process   umount ${destination}   OnError=Failed to release loopback mount
    Should Exist      ${destination}/directory/subdir/eicar.com

    ${source2} =       Set Variable  /tmp_test/eicar2.iso
    ${destination2} =  Set Variable  /tmp_test/iso_mount2
    Copy File  ${RESOURCES_PATH}/file_samples/eicar.iso  ${source2}
    Register Cleanup  Remove File   ${source2}
    Create Directory  ${destination2}
    Register Cleanup  Remove Directory   ${destination2}
    Run Shell Process   mount -o ro,loop ${source2} ${destination2}     OnError=Failed to create loopback mount
    Register Cleanup  Run Shell Process   umount ${destination2}   OnError=Failed to release loopback mount
    Should Exist      ${destination2}/directory/subdir/eicar.com

    Run Scan Now Scan
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now   timeout=240   interval=5
    AV Plugin Log Contains With Offset   Found 'EICAR-AV-Test' in '/tmp_test/iso_mount/directory/subdir/eicar.com'
    AV Plugin Log Contains With Offset  Found 'EICAR-AV-Test' in '/tmp_test/iso_mount2/directory/subdir/eicar.com'


AV Plugin Gets Customer ID
    ${customerIdFile1} =   Set Variable   ${AV_PLUGIN_PATH}/var/customer_id.txt
    ${customerIdFile2} =   Set Variable   ${AV_PLUGIN_PATH}/chroot${customerIdFile1}
    Remove Files   ${customerIdFile1}   ${customerIdFile2}

    ${policyContent} =   Get ALC Policy   userpassword=A  username=B
    Log   ${policyContent}
    Send Plugin Policy  av  alc  ${policyContent}

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
    Remove Files   ${customerIdFile1}   ${customerIdFile2}

    ${policyContent} =   Get ALC Policy
    ...   algorithm=AES256
    ...   userpassword=CCD8CFFX8bdCDFtU0+hv6MvL3FoxA0YeSNjJyZJWxz1b3uTsBu5p8GJqsfp3SAByOZw=
    ...   username=ABC123
    Log   ${policyContent}
    Send Plugin Policy  av  alc  ${policyContent}

    # md5(md5("ABC123:password"))
    ${expectedId} =   Set Variable   f5c33e370714d94e1d967e53ac4f0437

    Wait Until Created   ${customerIdFile1}   timeout=10sec
    ${customerId1} =   Get File   ${customerIdFile1}
    Should Be Equal   ${customerId1}   ${expectedId}

    Wait Until Created   ${customerIdFile2}   timeout=5sec
    ${customerId2} =   Get File   ${customerIdFile2}
    Should Be Equal   ${customerId2}   ${expectedId}


AV Plugin Gets Sxl Lookup Setting From SAV Policy
    ${susiStartupSettingsChrootFile} =   Set Variable   ${AV_PLUGIN_PATH}/chroot${SUSI_STARTUP_SETTINGS_FILE}
    Remove Files   ${SUSI_STARTUP_SETTINGS_FILE}   ${susiStartupSettingsChrootFile}

    ${policyContent} =   Get SAV Policy   sxlLookupEnabled=false
    Log    ${policyContent}
    Send Plugin Policy  av  sav  ${policyContent}
    Wait until scheduled scan updated With Offset

    ${expectedSusiStartupSettings} =   Set Variable   {"enableSxlLookup":false}

    Wait Until Created   ${SUSI_STARTUP_SETTINGS_FILE}   timeout=5sec
    ${susiStartupSettings} =   Get File   ${SUSI_STARTUP_SETTINGS_FILE}
    Should Be Equal   ${susiStartupSettings}   ${expectedSusiStartupSettings}

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
    Should Be equal As Integers  ${result.rc}  0
    Wait Until File exists  /run/netns/avtest
    ${exclusions} =  Configure Scan Exclusions Everything Else  /run/netns/
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/><onDemandScan><posixExclusions><filePathSet>${exclusions}</filePathSet></posixExclusions></onDemandScan></config>
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Policy  av  sav  ${policyContent}
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains With Offset  Completed scan Scan Now  timeout=180  interval=5
    AV Plugin Log Contains With Offset  Received new Action
    AV Plugin Log Contains With Offset  Evaluating Scan Now
    AV Plugin Log Contains With Offset  Starting scan Scan Now
    AV Plugin Log Contains With Offset  Publishing good threat health status after clean scan
    Check ScanNow Log Exists
    File Log Contains  ${SCANNOW_LOG_PATH}  Failed to scan /run/netns/avtest as it could not be read

Threat Detector Restarts If System File Contents Change
    copy file with permissions  ${TESTSYSPATH}  ${TESTSYSPATHBACKUP}
    Register On Fail   Revert System File To Original

    ${ORG_CONTENTS} =  Get File  ${TESTSYSPATH}  encoding_errors=replace
    Append To File  ${TESTSYSPATH}   "#NewLine"

    Wait Until AV Plugin Log Contains With Offset  System configuration updated for ${TESTSYSFILE}
    AV Plugin Log Should Not Contain With Offset  System configuration not changed for ${TESTSYSFILE}

    Wait until threat detector running
    mark sophos threat detector log

    Revert System File To Original
    Deregister On Fail   Revert System File To Original

    Wait Until AV Plugin Log Contains With Offset  System configuration updated for ${TESTSYSFILE}
    AV Plugin Log Should Not Contain With Offset  System configuration not changed for ${TESTSYSFILE}

    ${POSTTESTCONTENTS} =  Get File  ${TESTSYSPATH}  encoding_errors=replace
    Should Be Equal   ${POSTTESTCONTENTS}   ${ORG_CONTENTS}


Threat Detector Does Not Restart If System File Contents Do Not Change
    copy file with permissions  ${TESTSYSPATH}  ${TESTSYSPATHBACKUP}
    Revert System File To Original

    ## LINUXDAR-5249 - we now just check all files every time
    Wait Until AV Plugin Log Contains With Offset  System configuration not changed
    AV Plugin Log Should Not Contain With Offset  System configuration updated for ${TESTSYSFILE}


Threat Detector Restarts If Sometimes-symlinked System File Contents Change
    copy file  ${SOMETIMES_SYMLINKED_SYSPATH}  ${SOMETIMES_SYMLINKED_SYSPATHBACKUP}
    Register On Fail   Revert Sometimes-symlinked System File To Original

    ${ORG_CONTENTS} =  Get File  ${SOMETIMES_SYMLINKED_SYSPATH}  encoding_errors=replace
    Append To File  ${SOMETIMES_SYMLINKED_SYSPATH}   "#NewLine"

    Wait Until AV Plugin Log Contains With Offset  System configuration updated for ${SOMETIMES_SYMLINKED_SYSFILE}

    Wait until threat detector running
    mark sophos threat detector log

    Revert Sometimes-symlinked System File To Original
    Deregister On Fail   Revert Sometimes-symlinked System File To Original

    Wait Until AV Plugin Log Contains With Offset  System configuration updated for ${SOMETIMES_SYMLINKED_SYSFILE}

    ${POSTTESTCONTENTS} =  Get File  ${SOMETIMES_SYMLINKED_SYSPATH}  encoding_errors=replace
    Should Be Equal   ${POSTTESTCONTENTS}   ${ORG_CONTENTS}


Threat Detector Does Not Restart If Sometimes-symlinked System File Contents Do Not Change
    copy file  ${SOMETIMES_SYMLINKED_SYSPATH}  ${SOMETIMES_SYMLINKED_SYSPATHBACKUP}
    Revert Sometimes-symlinked System File To Original

    AV Plugin Log Should Not Contain With Offset  System configuration updated for ${SOMETIMES_SYMLINKED_SYSFILE}


*** Keywords ***
Start AV
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    mark av log
    mark sophos threat detector log
    ${threat_detector_handle} =  Start Process  ${SOPHOS_THREAT_DETECTOR_LAUNCHER}
    Set Suite Variable  ${THREAT_DETECTOR_PLUGIN_HANDLE}  ${threat_detector_handle}
    Register Cleanup   Terminate Process  ${THREAT_DETECTOR_PLUGIN_HANDLE}
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${handle}
    Register Cleanup   Terminate Process  ${AV_PLUGIN_HANDLE}
    Check AV Plugin Installed With Offset

AVBasic Suite Setup
    Start Fake Management If Required
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

    Remove File    ${AV_LOG_PATH}
    Remove File    ${THREAT_DETECTOR_LOG_PATH}
    Remove File    ${SUSI_DEBUG_LOG_PATH}*
    Start AV

Product Test Setup
    SystemFileWatcher.Start Watching System Files
    Register Cleanup      SystemFileWatcher.stop watching system files

    Start AV
    Component Test Setup
    Delete Eicars From Tmp
    mark av log
    mark sophos threat detector log
    mark susi debug log
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

    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  15 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule>${POLICY_7DAYS}<timeSet><time>${scanTime}</time></timeSet></schedule>
    ${scanObjectSet} =  Policy Fragment FS Types  networkDrives=false
    ${scanSet} =  Set Variable  <onDemandScan>${exclusions}<scanSet><scan><name>${remoteFSscanningDisabled}</name>${schedule}<settings>${scanObjectSet}</settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>
    Send Plugin Policy  av  sav  ${policyContent}
    Wait until scheduled scan updated With Offset
    Wait Until AV Plugin Log Contains With Offset  Starting scan ${remoteFSscanningDisabled}  timeout=120  interval=5
    Require Sophos Threat Detector Running
    Wait Until AV Plugin Log Contains With Offset  Completed scan ${remoteFSscanningDisabled}  timeout=240  interval=5
    File Should Exist  ${remoteFSscanningDisabled_log}
    File Log Should Not Contain  ${remoteFSscanningDisabled_log}  "${destination}/eicar.com" is infected with EICAR

    register on fail  dump log  ${remoteFSscanningEnabled_log}
    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  15 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule>${POLICY_7DAYS}<timeSet><time>${scanTime}</time></timeSet></schedule>
    ${scanObjectSet} =  Policy Fragment FS Types  networkDrives=true
    ${scanSet} =  Set Variable  <onDemandScan>${exclusions}<scanSet><scan><name>${remoteFSscanningEnabled}</name>${schedule}<settings>${scanObjectSet}</settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until AV Plugin Log Contains With Offset  Starting scan ${remoteFSscanningEnabled}  timeout=120  interval=5
    Require Sophos Threat Detector Running
    Wait Until AV Plugin Log Contains With Offset  Completed scan ${remoteFSscanningEnabled}  timeout=240  interval=5
    File Should Exist  ${remoteFSscanningEnabled_log}
    File Log Contains  ${remoteFSscanningEnabled_log}  "${destination}/eicar.com" is infected with EICAR

Revert System File To Original
    copy file with permissions  ${TESTSYSPATHBACKUP}  ${TESTSYSPATH}
    Remove File  ${TESTSYSPATHBACKUP}

Revert Sometimes-symlinked System File To Original
    copy file with permissions  ${SOMETIMES_SYMLINKED_SYSPATHBACKUP}  ${SOMETIMES_SYMLINKED_SYSPATH}
    Remove File  ${SOMETIMES_SYMLINKED_SYSPATHBACKUP}