*** Settings ***
Library         DateTime
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py

Resource    ../shared/ComponentSetup.robot
Resource    ../shared/AVResources.robot

*** Variables ***
${AV_PLUGIN_PATH}  ${COMPONENT_ROOT_PATH}
${AV_PLUGIN_BIN}   ${COMPONENT_BIN_PATH}
${AV_LOG_PATH}    ${AV_PLUGIN_PATH}/log/av.log

*** Test Cases ***
AV Plugin Can Receive Actions
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}

    Check AV Plugin Installed
    ${actionContent} =  Set Variable  This is an action test
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains  Received new Action

    ${result} =   Terminate Process  ${handle}

AV plugin Can Send Status
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed

    ${status}=  Get Plugin Status  av  sav
    Should Contain  ${status}   RevID

    ${telemetry}=  Get Plugin Telemetry  av
    Should Contain  ${telemetry}   Number of Scans

    ${result} =   Terminate Process  ${handle}


AV Plugin Can Process Scan Now
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    ${exclusions} =  Configure Scan Exclusions Everything Else  /tmp/
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/><onDemandScan><posixExclusions><filePathSet>${exclusions}</filePathSet></posixExclusions></onDemandScan></config>
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Policy  av  sav  ${policyContent}
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains  Completed scan scanNow
    AV Plugin Log Contains  Received new Action
    AV Plugin Log Contains  Starting Scan Now scan
    AV Plugin Log Contains  Starting scan scanNow
    Check ScanNow Log Exists

    ${result} =   Terminate Process  ${handle}


AV Plugin Will Fail Scan Now If No Policy
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed
    ${actionContent} =  Set Variable  <?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>
    Send Plugin Action  av  sav  corr123  ${actionContent}
    Wait Until AV Plugin Log Contains  Refusing to run invalid scan: INVALID
    AV Plugin Log Contains  Received new Action
    AV Plugin Log Contains  Starting Scan Now scan

    ${result} =   Terminate Process  ${handle}


AV Plugin Can Disable Scanning Of Remote File Systems
    ${source} =       Set Variable  /tmp/nfsshare
    ${destination} =  Set Variable  /mnt/nfsshare
    ${myscan_log} =   Set Variable  ${AV_PLUGIN_PATH}/log/MyScan.log
    Create Directory  ${source}
    Create File       ${source}/eicar.com    ${EICAR_STRING}
    Create Directory  ${destination}
    Create Local NFS Share   ${source}   ${destination}
    [Teardown]    Remove Local NFS Share   ${source}   ${destination}

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed

    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  60 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule><daySet><day>monday</day><day>tuesday</day><day>wednesday</day><day>thursday</day><day>friday</day><day>saturday</day><day>sunday</day></daySet><timeSet><time>${scanTime}</time></timeSet></schedule>
    ${scanSet} =  Set Variable  <onDemandScan><scanSet><scan><name>MyScan</name>${schedule}<settings><scanObjectSet><CDDVDDrives>false</CDDVDDrives><hardDrives>false</hardDrives><networkDrives>false</networkDrives><removableDrives>false</removableDrives></scanObjectSet></settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>
    Send Plugin Policy  av  sav  ${policyContent}
    Wait Until AV Plugin Log Contains  Completed scan MyScan  timeout=120
    AV Plugin Log Contains  Starting scan MyScan
    File Should Exist  ${myscan_log}
    File Log Should Not Contain  ${myscan_log}  "${destination}" is infected with EICAR

    ${result} =   Terminate Process  ${handle}


AV Plugin Can Exclude Filepaths From Scheduled Scans
    ${eicar_path} =  Set Variable  /tmp/eicar.com
    Create File      ${eicar_path}    ${EICAR_STRING}
    ${myscan_log} =   Set Variable  ${AV_PLUGIN_PATH}/log/MyScan.log

    ${handle} =  Start Process  ${AV_PLUGIN_BIN}
    Check AV Plugin Installed

    ${currentTime} =  Get Current Date
    ${scanTime} =  Add Time To Date  ${currentTime}  60 seconds  result_format=%H:%M:%S
    ${schedule} =  Set Variable  <schedule><daySet><day>monday</day><day>tuesday</day><day>wednesday</day><day>thursday</day><day>friday</day><day>saturday</day><day>sunday</day></daySet><timeSet><time>${scanTime}</time></timeSet></schedule>
    ${allButTmp} =  Configure Scan Exclusions Everything Else  /tmp/
    ${exclusions} =  Set Variable  <posixExclusions><filePathSet>${allButTmp}<filePath>${eicar_path}</filePath></filePathSet></posixExclusions>
    ${scanSet} =  Set Variable  <onDemandScan>${exclusions}<scanSet><scan><name>MyScan</name>${schedule}<settings><scanObjectSet><CDDVDDrives>false</CDDVDDrives><hardDrives>true</hardDrives><networkDrives>false</networkDrives><removableDrives>false</removableDrives></scanObjectSet></settings></scan></scanSet></onDemandScan>
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${scanSet}</config>
    Send Plugin Policy  av  sav  ${policyContent}
    Wait Until AV Plugin Log Contains  Completed scan MyScan  timeout=120
    AV Plugin Log Contains  Starting scan MyScan
    File Should Exist  ${myscan_log}
    File Log Should Not Contain  ${myscan_log}  "${eicar_path}" is infected with EICAR

    ${result} =   Terminate Process  ${handle}