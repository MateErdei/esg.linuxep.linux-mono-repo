*** Settings ***
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
    ${onDemand} =  Configure Scan Exclusions Everything Else  /tmp/
    ${policyContent} =  Set Variable  <?xml version="1.0"?><config xmlns="http://www.sophos.com/EE/EESavConfiguration"><csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>${onDemand}</config>
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
