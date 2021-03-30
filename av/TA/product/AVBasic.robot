*** Settings ***
Documentation   Product tests for AVP
Force Tags      PRODUCT  AV_BASIC
Library         Collections
Library         DateTime
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/serialisationtools/CapnpHelper.py

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot
Resource    ../shared/BaseResources.robot
Resource    ../shared/FakeManagementResources.robot
Resource    ../shared/SambaResources.robot

Suite Setup    AVBasic Suite Setup

Test Setup     Product Test Setup
Test Teardown  Product Test Teardown

*** Variables ***
${AV_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${AV_LOG_PATH}    ${AV_PLUGIN_PATH}/log/av.log
${HANDLE}

*** Test Cases ***
AV Plugin Can Receive Actions
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}

    Check AV Plugin Installed
    ${actionContent} =  Set Variable  This is an action test
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains  Received new Action

    ${result} =   Terminate Process  ${handle}

AV plugin Can Send Status
    [Tags]    PRODUCT  AV_BASIC_STATUS
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    ${version} =  Get Version Number From Ini File  ${COMPONENT_ROOT_PATH}/VERSION.ini

    ${status}=  Get Plugin Status  av  SAV
    Should Contain  ${status}   RevID=""
    Should Contain  ${status}   Res="NoRef"
    Should Contain  ${status}   <product-version>${version}</product-version>

    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="123" policyType="2"/></config>
    Send Plugin Policy  av  sav  ${policyContent}

    Wait For Plugin Status  av  SAV  RevID="123"  Res="Same"  <product-version>${version}</product-version>

    ${telemetryString}=  Get Plugin Telemetry  av

    ${telemetryJson}=    Evaluate     json.loads("""${telemetryString}""")    json

    Dictionary Should Contain Key   ${telemetryJson}   lr-data-hash
    Dictionary Should Contain Key   ${telemetryJson}   ml-pe-model-hash
    Dictionary Should Contain Key   ${telemetryJson}   version

    ${result} =   Terminate Process  ${handle}


AV Plugin Can Process Scan Now
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    ${exclusions} =  Configure Scan Exclusions Everything Else  /tmp_test/
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/><onDemandScan><posixExclusions><filePathSet>${exclusions}</filePathSet></posixExclusions></onDemandScan></config>
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Policy  av  sav  ${policyContent}
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=180  interval=5
    AV Plugin Log Contains  Received new Action
    AV Plugin Log Contains  Evaluating Scan Now
    AV Plugin Log Contains  Starting scan Scan Now
    Check ScanNow Log Exists

    ${result} =   Terminate Process  ${handle}


AV Plugin Scan Now Updates Telemetry Count
    ${HANDLE} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed

    # reset telemetry count
    ${telemetryString}=  Get Plugin Telemetry  av
    Log   ${telemetryString}

    ${exclusions} =  Configure Scan Exclusions Everything Else  /tmp_test/
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/><onDemandScan><posixExclusions><filePathSet>${exclusions}</filePathSet></posixExclusions></onDemandScan></config>
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Policy  av  sav  ${policyContent}
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=180  interval=5
    AV Plugin Log Contains  Received new Action
    AV Plugin Log Contains  Evaluating Scan Now
    AV Plugin Log Contains  Starting scan Scan Now
    Check ScanNow Log Exists

    ${telemetryString}=  Get Plugin Telemetry  av
    Log   ${telemetryString}
    ${telemetryJson}=    Evaluate     json.loads("""${telemetryString}""")    json

    Dictionary Should Contain Item   ${telemetryJson}   scan-now-count   1

    ${result} =   Terminate Process  ${handle}


Scan Now Configuration Is Correct
    Use Fake AVScanner
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    Run Scan Now Scan
    Check Scan Now Configuration File is Correct

    ${result} =   Terminate Process  ${handle}


Scheduled Scan Configuration Is Correct
    Use Fake AVScanner
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    Run Scheduled Scan
    Check Scheduled Scan Configuration File is Correct

    ${result} =   Terminate Process  ${handle}


Scan Now Excludes Files And Directories As Expected
    Create Directory  /directory_excluded/
    Create Directory  /file_excluded/

    Create File  /eicar.com                     ${EICAR_STRING}
    Create File  /directory_excluded/eicar.com  ${EICAR_STRING}
    Create File  /file_excluded/eicar.com       ${EICAR_STRING}
    Register Cleanup  Remove Directory  /file_excluded  recursive=True
    Register Cleanup  Remove Directory  /directory_excluded  recursive=True
    Register Cleanup  Remove Files  /eicar.com  /directory_excluded/eicar.com  /file_excluded/eicar.com

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    Run Scan Now Scan For Excluded Files Test

    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=240  interval=5

    File Log Contains             ${SCANNOW_LOG_PATH}        Excluding file: /eicar.com
    File Log Contains             ${SCANNOW_LOG_PATH}        "/file_excluded/eicar.com" is infected with EICAR-AV-Test
    File Log Contains             ${SCANNOW_LOG_PATH}        Excluding directory: /directory_excluded/
    File Log Should Not Contain   ${SCANNOW_LOG_PATH}        "/directory_excluded/eicar.com" is infected with EICAR-AV-Test
    File Log Should Not Contain   ${SCANNOW_LOG_PATH}        Excluding file: /directory_excluded/eicar.com

    ${result} =   Terminate Process  ${handle}

Scan Now Logs Should Be As Expected
    Create Directory  /file_excluded/

    Create File  /file_excluded/eicar.com       ${EICAR_STRING}
    Register Cleanup  Remove Files  /file_excluded/eicar.com
    Register Cleanup  Remove Directory  /file_excluded  recursive=True

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    Run Scan Now Scan For Excluded Files Test

    AV Plugin Log Contains  Received new Action
    Wait Until AV Plugin Log Contains  Evaluating Scan Now
    Wait Until AV Plugin Log Contains  Starting scan Scan Now  timeout=10

    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=240  interval=5

    File Log Contains             ${SCANNOW_LOG_PATH}        End of Scan Summary:
    File Log Contains             ${SCANNOW_LOG_PATH}        1 file out of
    File Log Contains             ${SCANNOW_LOG_PATH}        1 EICAR-AV-Test infection discovered.
    File Log Should Not Contain   ${AV_LOG_PATH}             Notify trimmed output

    ${result} =   Terminate Process  ${handle}


AV Plugin Will Fail Scan Now If No Policy
    Register Cleanup  Remove File  ${MCS_ACTION_DIRECTORY}/ScanNow_Action*

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains  Refusing to run invalid scan: INVALID
    AV Plugin Log Contains  Received new Action
    AV Plugin Log Contains  Evaluating Scan Now

    ${result} =   Terminate Process  ${handle}


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

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed

    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  65 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule>${POLICY_7DAYS}<timeSet><time>${scanTime}</time></timeSet></schedule>
    ${scanObjectSet} =  Policy Fragment FS Types  hardDrives=true
    ${scanSet} =  Set Variable  <onDemandScan>${exclusions}<scanSet><scan><name>${scanName}</name>${schedule}<settings>${scanObjectSet}</settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>
    Send Plugin Policy  av  sav  ${policyContent}
    Wait Until AV Plugin Log Contains  Scheduled Scan: ${scanName}   timeout=30
    Wait Until AV Plugin Log Contains  Starting scan ${scanName}     timeout=90
    Wait Until AV Plugin Log Contains  Completed scan ${scanName}    timeout=60
    File Should Exist  ${scanName_log}
    File Log Contains  ${scanName_log}  "${destination}/eicar.com" is infected with EICAR

    ${content} =  Get File   ${scanName_log}  encoding_errors=replace
    ${lines} =  Get Lines Containing String     ${content}  "${destination}/eicar.com" is infected with EICAR
    ${count} =  Get Line Count   ${lines}
    Should Be Equal As Integers  ${count}  ${1}

    ${result} =   Terminate Process  ${handle}

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
    [Tags]  SMB
    ${source} =       Set Variable  /tmp_test/smbshare
    ${destination} =  Set Variable  /testmnt/smbshare
    Create Directory  ${source}
    Create File       ${source}/eicar.com    ${EICAR_STRING}
    Register Cleanup  Remove File      ${source}/eicar.com
    Create Directory  ${destination}
    Create Local SMB Share   ${source}   ${destination}
    Register Cleanup  Remove Local SMB Share   ${source}   ${destination}

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

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed

    run_on_failure  dump_scheduled_scan_log

    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  60 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule><daySet><day>monday</day><day>tuesday</day><day>wednesday</day><day>thursday</day><day>friday</day><day>saturday</day><day>sunday</day></daySet><timeSet><time>${scanTime}</time></timeSet></schedule>
    ${allButTmp} =  Configure Scan Exclusions Everything Else  /tmp_test/
    ${exclusions} =  Set Variable  <posixExclusions><filePathSet>${allButTmp}<filePath>${eicar_path1}</filePath><filePath>/tmp_test/eicar.?</filePath><filePath>/tmp_test/*.txt</filePath><filePath>eicarStr</filePath></filePathSet></posixExclusions>
    ${scanSet} =  Set Variable  <onDemandScan>${exclusions}<scanSet><scan><name>MyScan</name>${schedule}<settings><scanObjectSet><CDDVDDrives>false</CDDVDDrives><hardDrives>true</hardDrives><networkDrives>false</networkDrives><removableDrives>false</removableDrives></scanObjectSet></settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>
    Send Plugin Policy  av  sav  ${policyContent}
    Wait Until AV Plugin Log Contains  Completed scan MyScan  timeout=240  interval=5
    AV Plugin Log Contains  Starting scan MyScan
    File Should Exist  ${myscan_log}
    File Log Should Not Contain  ${myscan_log}  "${eicar_path1}" is infected with EICAR-AV-Test
    File Log Should Not Contain  ${myscan_log}  "${eicar_path2}" is infected with EICAR-AV-Test
    File Log Should Not Contain  ${myscan_log}  "${eicar_path3}" is infected with EICAR-AV-Test
    File Log Should Not Contain  ${myscan_log}  "${eicar_path4}" is infected with EICAR-AV-Test
    File Log Contains            ${myscan_log}  "${eicar_path5}" is infected with EICAR-AV-Test

    ${result} =   Terminate Process  ${handle}


AV Plugin Scan of Infected File Increases Threat Eicar Count
    Create File      /tmp_test/eicar.com    ${EICAR_STRING}
    Register Cleanup  Remove File  /tmp_test/eicar.com
    Remove Files      /file_excluded/eicar.com  /tmp_test/smbshare/eicar.com

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed

    # Run telemetry to reset counters to 0
    ${telemetryString}=  Get Plugin Telemetry  av
    ${telemetryJson}=    Evaluate     json.loads("""${telemetryString}""")    json

    Run Scan Now Scan

    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=240  interval=5

    ${telemetryString}=  Get Plugin Telemetry  av
    ${telemetryJson}=    Evaluate     json.loads("""${telemetryString}""")    json
    Log   ${telemetryJson}
    Dictionary Should Contain Item   ${telemetryJson}   threat-eicar-count   1

    ${result} =   Terminate Process  ${handle}


AV Plugin Scan Now Does Not Detect PUA
    Create File      /tmp_test/eicar_pua.com    ${EICAR_PUA_STRING}

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed

    Run Scan Now Scan

    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=240  interval=5

    AV Plugin Log Does Not Contain  /tmp_test/eicar_pua.com

    File Log Should Not Contain  ${AV_PLUGIN_PATH}/log/Scan Now.log  "/tmp_test/eicar_pua.com" is infected

    ${result} =   Terminate Process  ${handle}


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

    ${handle} =       Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    Run Scan Now Scan
    Wait Until AV Plugin Log Contains   Completed scan Scan Now   timeout=240   interval=5

    ${content} =      Get File   ${AV_LOG_PATH}   encoding_errors=replace
    ${lines} =        Get Lines Containing String    ${content}   Found 'EICAR-AV-Test'
    ${count} =        Get Line Count   ${lines}
    Should Be Equal As Integers  ${1}  ${count}

    ${result} =       Terminate Process  ${handle}


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

    ${handle} =       Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    Run Scan Now Scan
    Wait Until AV Plugin Log Contains   Completed scan Scan Now   timeout=240   interval=5
    AV Plugin Log Contains   Found 'EICAR-AV-Test' in '/tmp_test/iso_mount/directory/subdir/eicar.com'
    ${result} =       Terminate Process  ${handle}


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


    ${handle} =       Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    Run Scan Now Scan
    Wait Until AV Plugin Log Contains   Completed scan Scan Now   timeout=240   interval=5
    AV Plugin Log Contains   Found 'EICAR-AV-Test' in '/tmp_test/iso_mount/directory/subdir/eicar.com'
    AV Plugin Log Contains   Found 'EICAR-AV-Test' in '/tmp_test/iso_mount2/directory/subdir/eicar.com'
    ${result} =       Terminate Process  ${handle}


AV Plugin Gets Customer ID
    ${customerIdFile1} =   Set Variable   ${AV_PLUGIN_PATH}/var/customer_id.txt
    ${customerIdFile2} =   Set Variable   ${AV_PLUGIN_PATH}/chroot${customerIdFile1}
    Remove Files   ${customerIdFile1}   ${customerIdFile2}

    ${handle} =   Start Process  ${AV_PLUGIN_BIN}
    Register Cleanup   Terminate Process  ${handle}
    Check AV Plugin Installed

    ${policyContent} =   Get ALC Policy   userpassword=A  username=B
    Log   ${policyContent}
    Send Plugin Policy  av  alc  ${policyContent}

    ${expectedId} =   Set Variable   a1c0f318e58aad6bf90d07cabda54b7d

    Wait Until Created   ${customerIdFile1}   timeout=5sec
    ${customerId1} =   Get File   ${customerIdFile1}
    Should Be Equal   ${customerId1}   ${expectedId}

    Wait Until Created   ${customerIdFile2}   timeout=5sec
    ${customerId2} =   Get File   ${customerIdFile2}
    Should Be Equal   ${customerId2}   ${expectedId}


AV Plugin Gets Customer ID from Obfuscated Creds
    ${customerIdFile1} =   Set Variable   ${AV_PLUGIN_PATH}/var/customer_id.txt
    ${customerIdFile2} =   Set Variable   ${AV_PLUGIN_PATH}/chroot${customerIdFile1}
    Remove Files   ${customerIdFile1}   ${customerIdFile2}

    ${handle} =   Start Process  ${AV_PLUGIN_BIN}
    Register Cleanup   Terminate Process  ${handle}
    Check AV Plugin Installed

    ${policyContent} =   Get ALC Policy
    ...   algorithm=AES256
    ...   userpassword=CCD8CFFX8bdCDFtU0+hv6MvL3FoxA0YeSNjJyZJWxz1b3uTsBu5p8GJqsfp3SAByOZw=
    ...   username=ABC123
    Log   ${policyContent}
    Send Plugin Policy  av  alc  ${policyContent}

    # md5(md5("ABC123:password"))
    ${expectedId} =   Set Variable   f5c33e370714d94e1d967e53ac4f0437

    Wait Until Created   ${customerIdFile1}   timeout=5sec
    ${customerId1} =   Get File   ${customerIdFile1}
    Should Be Equal   ${customerId1}   ${expectedId}

    Wait Until Created   ${customerIdFile2}   timeout=5sec
    ${customerId2} =   Get File   ${customerIdFile2}
    Should Be Equal   ${customerId2}   ${expectedId}


AV Plugin Gets Sxl Lookup Setting From SAV Policy
    ${susiStartupSettingsChrootFile} =   Set Variable   ${AV_PLUGIN_PATH}/chroot${SUSI_STARTUP_SETTINGS_FILE}
    Remove Files   ${SUSI_STARTUP_SETTINGS_FILE}   ${susiStartupSettingsChrootFile}

    ${handle} =   Start Process  ${AV_PLUGIN_BIN}
    Register Cleanup   Terminate Process  ${handle}
    Check AV Plugin Installed

    ${policyContent} =   Get SAV Policy   sxlLookupEnabled=false
    Log    ${policyContent}
    Send Plugin Policy  av  sav  ${policyContent}

    ${expectedSusiStartupSettings} =   Set Variable   {"enableSxlLookup":false}

    Wait Until Created   ${SUSI_STARTUP_SETTINGS_FILE}   timeout=5sec
    ${susiStartupSettings} =   Get File   ${SUSI_STARTUP_SETTINGS_FILE}
    Should Be Equal   ${susiStartupSettings}   ${expectedSusiStartupSettings}

AV Plugin requests policies at startup
    ${handle} =   Start Process  ${AV_PLUGIN_BIN}
    Register Cleanup   Terminate Process  ${handle}
    Check AV Plugin Installed

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  FakeManagement Log Contains   Received policy request: APPID=SAV

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  FakeManagement Log Contains   Received policy request: APPID=ALC


AV Plugin restarts threat detector on customer id change
    ${handle} =   Start Process  ${AV_PLUGIN_BIN}
    Register Cleanup   Terminate Process  ${handle}
    Check AV Plugin Installed

    Mark AV Log
    Mark Sophos Threat Detector Log
    ${pid} =   Record Sophos Threat Detector PID

    ${id1} =   Generate Random String
    ${policyContent} =   Get ALC Policy   revid=${id1}  userpassword=${id1}  username=${id1}
    Log   ${policyContent}
    Send Plugin Policy  av  alc  ${policyContent}

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system configuration has changed
    Wait Until Sophos Threat Detector Log Contains With Offset   UnixSocket <> Starting listening on socket
    Check Sophos Threat Detector has different PID   ${pid}

    # change revid only, threat_detector should not restart
    Mark AV Log
    Mark Sophos Threat Detector Log
    ${pid} =   Record Sophos Threat Detector PID

    ${id2} =   Generate Random String
    ${policyContent} =   Get ALC Policy   revid=${id2}  userpassword=${id1}  username=${id1}
    Log   ${policyContent}
    Send Plugin Policy  av  alc  ${policyContent}

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Run Keyword And Expect Error
    ...   Keyword 'AV Plugin Log Contains With Offset' failed after retrying for 5 seconds.*
    ...   Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system configuration has changed   timeout=5
    Check Sophos Threat Detector has same PID   ${pid}

    # change credentials, threat_detector should restart
    Mark AV Log
    Mark Sophos Threat Detector Log
    ${pid} =   Record Sophos Threat Detector PID

    ${id3} =   Generate Random String
    ${policyContent} =   Get ALC Policy   revid=${id3}  userpassword=${id3}  username=${id3}
    Log   ${policyContent}
    Send Plugin Policy  av  alc  ${policyContent}

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system configuration has changed
    Wait Until Sophos Threat Detector Log Contains With Offset   UnixSocket <> Starting listening on socket
    Check Sophos Threat Detector has different PID   ${pid}

AV Plugin sets default if susi startup settings permissions incorrect
    ${handle} =   Start Process  ${AV_PLUGIN_BIN}
    Register Cleanup   Terminate Process  ${handle}
    Check AV Plugin Installed

    Mark AV Log
    Mark Sophos Threat Detector Log

    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=false
    Log   ${policyContent}
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until Sophos Threat Detector Log Contains With Offset   UnixSocket <> Starting listening on socket

    Run Process  chmod  000  ${SUSI_STARTUP_SETTINGS_FILE}

    Terminate Process  ${handle}
    ${handle} =   Start Process  ${AV_PLUGIN_BIN}

    Wait Until Sophos Threat Detector Log Contains With Offset   UnixSocket <> Starting listening on socket
    Wait Until Sophos Threat Detector Log Contains With Offset   Turning Live Protection on as default - no susi startup settings found


AV Plugin restarts threat detector on susi startup settings change
    ${handle} =   Start Process  ${AV_PLUGIN_BIN}
    Register Cleanup   Terminate Process  ${handle}
    Check AV Plugin Installed

    Mark AV Log
    Mark Sophos Threat Detector Log
    ${pid} =   Record Sophos Threat Detector PID

    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=false
    Log   ${policyContent}
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system configuration has changed
    Wait Until Sophos Threat Detector Log Contains With Offset   UnixSocket <> Starting listening on socket
    Check Sophos Threat Detector has different PID   ${pid}

    # don't change lookup setting, threat_detector should not restart
    Mark AV Log
    Mark Sophos Threat Detector Log
    ${pid} =   Record Sophos Threat Detector PID

    ${id2} =   Generate Random String
    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=false
    Log   ${policyContent}
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Run Keyword And Expect Error
    ...   Keyword 'AV Plugin Log Contains With Offset' failed after retrying for 5 seconds.*
    ...   Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system configuration has changed   timeout=5
    Check Sophos Threat Detector has same PID   ${pid}

    # change lookup setting, threat_detector should restart
    Mark AV Log
    Mark Sophos Threat Detector Log
    ${pid} =   Record Sophos Threat Detector PID

    ${id3} =   Generate Random String
    ${policyContent} =   Get SAV Policy  sxlLookupEnabled=true
    Log   ${policyContent}
    Send Plugin Policy  av  sav  ${policyContent}

    Wait Until AV Plugin Log Contains With Offset   Received new policy
    Wait Until AV Plugin Log Contains With Offset   Restarting sophos_threat_detector as the system configuration has changed
    Wait Until Sophos Threat Detector Log Contains With Offset   UnixSocket <> Starting listening on socket
    Check Sophos Threat Detector has different PID   ${pid}


*** Keywords ***

AVBasic Suite Setup
    Start Fake Management If Required

Product Test Setup
    Component Test Setup
    Delete Eicars From Tmp

Product Test Teardown
    ${usingFakeAVScanner} =  Get Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${COMPONENT_ROOT_PATH}/log/${COMPONENT_NAME}.log  encoding_errors=replace
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${FAKEMANAGEMENT_AGENT_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${THREAT_DETECTOR_LOG_PATH}  encoding_errors=replace
    Run Keyword If  '${usingFakeAVScanner}'=='true'  Undo Use Fake AVScanner
    Delete Eicars From Tmp
    run teardown functions
    Component Test TearDown

Test Remote Share
    [Arguments]  ${destination}
    Should exist  ${destination}/eicar.com
    ${remoteFSscanningDisabled} =   Set Variable  remoteFSscanningDisabled
    ${remoteFSscanningEnabled} =    Set Variable  remoteFSscanningEnabled
    ${remoteFSscanningDisabled_log} =   Set Variable  ${AV_PLUGIN_PATH}/log/${remoteFSscanningDisabled}.log
    ${remoteFSscanningEnabled_log} =   Set Variable  ${AV_PLUGIN_PATH}/log/${remoteFSscanningEnabled}.log

    ${allButTmp} =  Configure Scan Exclusions Everything Else  /testmnt/
    ${exclusions} =  Set Variable  <posixExclusions><filePathSet>${allButTmp}</filePathSet></posixExclusions>

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed

    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  60 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule>${POLICY_7DAYS}<timeSet><time>${scanTime}</time></timeSet></schedule>
    ${scanObjectSet} =  Policy Fragment FS Types  networkDrives=false
    ${scanSet} =  Set Variable  <onDemandScan>${exclusions}<scanSet><scan><name>${remoteFSscanningDisabled}</name>${schedule}<settings>${scanObjectSet}</settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>
    Send Plugin Policy  av  sav  ${policyContent}
    Wait Until AV Plugin Log Contains  Completed scan ${remoteFSscanningDisabled}  timeout=240  interval=5
    AV Plugin Log Contains  Starting scan ${remoteFSscanningDisabled}
    File Should Exist  ${remoteFSscanningDisabled_log}
    File Log Should Not Contain  ${remoteFSscanningDisabled_log}  "${destination}/eicar.com" is infected with EICAR

    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  60 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule>${POLICY_7DAYS}<timeSet><time>${scanTime}</time></timeSet></schedule>
    ${scanObjectSet} =  Policy Fragment FS Types  networkDrives=true
    ${scanSet} =  Set Variable  <onDemandScan>${exclusions}<scanSet><scan><name>${remoteFSscanningEnabled}</name>${schedule}<settings>${scanObjectSet}</settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>
    Send Plugin Policy  av  sav  ${policyContent}
    Wait Until AV Plugin Log Contains  Completed scan ${remoteFSscanningEnabled}  timeout=240  interval=5
    AV Plugin Log Contains  Starting scan ${remoteFSscanningEnabled}
    File Should Exist  ${remoteFSscanningEnabled_log}
    File Log Contains  ${remoteFSscanningEnabled_log}  "${destination}/eicar.com" is infected with EICAR

    ${result} =   Terminate Process  ${handle}

Get ALC Policy
    [Arguments]  ${revid}=${EMPTY}  ${algorithm}=Clear  ${username}=B  ${userpassword}=A
    ${policyContent} =  Catenate   SEPARATOR=${\n}
    ...   <?xml version="1.0"?>
    ...   <AUConfigurations xmlns:csc="com.sophos\\msys\\csc" xmlns="http://www.sophos.com/EE/AUConfig">
    ...     <csc:Comp RevID="${revid}" policyType="1"/>
    ...     <AUConfig>
    ...       <primary_location>
    ...         <server Algorithm="${algorithm}" UserPassword="${userpassword}" UserName="${username}"/>
    ...       </primary_location>
    ...     </AUConfig>
    ...   </AUConfigurations>
    ${policyContent} =   Replace Variables   ${policyContent}
    [Return]   ${policyContent}

Get SAV Policy
    [Arguments]  ${revid}=${EMPTY}  ${sxlLookupEnabled}=A
    ${policyContent} =  Catenate   SEPARATOR=${\n}
    ...   <?xml version="1.0"?>
    ...   <config>
    ...       <csc:Comp RevID="${revid}" policyType="2"/>
    ...       <detectionFeedback>
    ...           <sendData>${sxlLookupEnabled}</sendData>
    ...       </detectionFeedback>
    ...   </config>
    ${policyContent} =   Replace Variables   ${policyContent}
    [Return]   ${policyContent}