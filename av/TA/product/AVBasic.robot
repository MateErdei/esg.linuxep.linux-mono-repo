*** Settings ***
Documentation   Product tests for AVP
Default Tags    PRODUCT  AV_BASIC
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
    ${exclusions} =  Configure Scan Exclusions Everything Else  /tmp/
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/><onDemandScan><posixExclusions><filePathSet>${exclusions}</filePathSet></posixExclusions></onDemandScan></config>
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Policy  av  sav  ${policyContent}
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=180
    AV Plugin Log Contains  Received new Action
    AV Plugin Log Contains  Evaluating Scan Now
    AV Plugin Log Contains  Starting scan Scan Now
    Check ScanNow Log Exists

    ${result} =   Terminate Process  ${handle}


AV Plugin Scan Now Updates Telemetry Count
    ${HANDLE} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    ${exclusions} =  Configure Scan Exclusions Everything Else  /tmp/
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/><onDemandScan><posixExclusions><filePathSet>${exclusions}</filePathSet></posixExclusions></onDemandScan></config>
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Policy  av  sav  ${policyContent}
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=180
    AV Plugin Log Contains  Received new Action
    AV Plugin Log Contains  Evaluating Scan Now
    AV Plugin Log Contains  Starting scan Scan Now
    Check ScanNow Log Exists

    ${telemetryString}=  Get Plugin Telemetry  av

    Log To Console      ${telemetryString}

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
    # TODO LINUXDAR-1482 Change Send Sav Policy With Imminent Scheduled Scan To Base (see comment in method)
    Run Scheduled Scan
    Check Scheduled Scan Configuration File is Correct

    ${result} =   Terminate Process  ${handle}

AV Plugin Will Fail Scan Now If No Policy
    [Teardown]  Run Keywords    Remove File  ${MCS_ACTION_DIRECTORY}/ScanNow_Action*
    ...         AND             Product Test Teardown

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains  Refusing to run invalid scan: INVALID
    AV Plugin Log Contains  Received new Action
    AV Plugin Log Contains  Evaluating Scan Now

    ${result} =   Terminate Process  ${handle}


AV Plugin Can Disable Scanning Of Remote File Systems
    [Tags]  NFS
    ${source} =       Set Variable  /tmp/nfsshare
    ${destination} =  Set Variable  /mnt/nfsshare
    ${remoteFSscanningDisabled} =   Set Variable  remoteFSscanningDisabled
    ${remoteFSscanningEnabled} =    Set Variable  remoteFSscanningEnabled
    ${remoteFSscanningDisabled_log} =   Set Variable  ${AV_PLUGIN_PATH}/log/${remoteFSscanningDisabled}.log
    ${remoteFSscanningEnabled_log} =   Set Variable  ${AV_PLUGIN_PATH}/log/${remoteFSscanningEnabled}.log
    Create Directory  ${source}
    Create File       ${source}/eicar.com    ${EICAR_STRING}
    Create Directory  ${destination}
    Create Local NFS Share   ${source}   ${destination}
    [Teardown]    Remove Local NFS Share   ${source}   ${destination}

    ${allButTmp} =  Configure Scan Exclusions Everything Else  /mnt/
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
    Wait Until AV Plugin Log Contains  Completed scan ${remoteFSscanningDisabled}  timeout=120
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
    Wait Until AV Plugin Log Contains  Completed scan ${remoteFSscanningEnabled}  timeout=120
    AV Plugin Log Contains  Starting scan ${remoteFSscanningEnabled}
    File Should Exist  ${remoteFSscanningEnabled_log}
    File Log Contains  ${remoteFSscanningEnabled_log}  "${destination}/eicar.com" is infected with EICAR

    ${result} =   Terminate Process  ${handle}


AV Plugin Can Exclude Filepaths From Scheduled Scans
    [Teardown]  Run Keywords    Delete Eicars From Tmp
    ...                         Product Test Teardown

    ${eicar_path1} =  Set Variable  /tmp/eicar.com
    ${eicar_path2} =  Set Variable  /tmp/eicar.1
    ${eicar_path3} =  Set Variable  /tmp/eicar.txt
    ${eicar_path4} =  Set Variable  /tmp/eicarStr
    ${eicar_path5} =  Set Variable  /tmp/eicar
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
    ${allButTmp} =  Configure Scan Exclusions Everything Else  /tmp/
    ${exclusions} =  Set Variable  <posixExclusions><filePathSet>${allButTmp}<filePath>${eicar_path1}</filePath><filePath>/tmp/eicar.?</filePath><filePath>/tmp/*.txt</filePath><filePath>eicarStr</filePath></filePathSet></posixExclusions>
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
    [Teardown]  Run Keywords    Delete Eicars From Tmp
    ...                         Product Test Teardown

    Create File      /tmp/eicar.com    ${EICAR_STRING}

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed

    # Run telemetry to reset counters to 0
    ${telemetryString}=  Get Plugin Telemetry  av
    ${telemetryJson}=    Evaluate     json.loads("""${telemetryString}""")    json

    Run Scan Now Scan

    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=240  interval=5

    ${telemetryString}=  Get Plugin Telemetry  av
    ${telemetryJson}=    Evaluate     json.loads("""${telemetryString}""")    json

    Log To Console  ${telemetryJson}

    Dictionary Should Contain Item   ${telemetryJson}   threat-eicar-count   1

    ${result} =   Terminate Process  ${handle}


AV Plugin Scan Now Does Not Detect PUA
    [Teardown]  Run Keywords    Delete Eicars From Tmp
    ...                         Product Test Teardown

    Create File      /tmp/eicar_pua.com    ${EICAR_PUA_STRING}

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed

    Run Scan Now Scan

    Wait Until AV Plugin Log Contains  Completed scan Scan Now  timeout=240  interval=5

    AV Plugin Log Does Not Contain  /tmp/eicar_pua.com

    File Log Should Not Contain  ${AV_PLUGIN_PATH}/log/Scan Now.log  "/tmp/eicar_pua.com" is infected

    ${result} =   Terminate Process  ${handle}


*** Keywords ***

Product Test Teardown
    ${usingFakeAVScanner} =  Get Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${COMPONENT_ROOT_PATH}/log/${COMPONENT_NAME}.log  encoding_errors=replace
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${FAKEMANAGEMENT_AGENT_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${THREAT_DETECTOR_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed  run_failure_functions_if_failed
    Run Keyword If  '${usingFakeAVScanner}'=='true'  Undo Use Fake AVScanner