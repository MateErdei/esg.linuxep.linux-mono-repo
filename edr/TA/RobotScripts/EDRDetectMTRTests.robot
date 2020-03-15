*** Settings ***
Documentation    Tests to handle how EDR changes the configuration for Osquery when it detects that MTR is meant to be installed.

Library         Process
Library         OperatingSystem
Library         String
Library         ../Libs/FakeManagement.py

Resource        EDRResources.robot
Resource        ComponentSetup.robot

Suite Setup     Install With Base SDDS  enableAuditConfig=True  preInstallALCPolicy=True
Suite Teardown  Uninstall And Revert Setup

Test Setup      No Operation
Test Teardown   EDR And Base Teardown


*** Test Cases ***
EDR By Default Will Configure Audit Option
    Check EDR Plugin Installed With Base
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Configured To Collect Audit Data

    Wait For EDR Log   edr <> No MTR Detected
    Wait For EDR Log   edr <> Run osquery process

    #give edr time to run
    Sleep  20 secs

    Check MTR in ALC Policy Forces Disable Audit Data Collection
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Running Instance of Osquery Configured To Not Collect Audit Data

    Wait For EDR Log  edr <> Detected MTR is enabled
    Wait For EDR Log  edr <> Restarting osquery
    Wait For EDR Log  Issue request to stop to osquery


    Sleep  15 secs
    #Run Keyword And Expect Error   GLOB

    #wait for the expected restart
    Wait For EDR Log  edr <> Restarting osquery
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  EDR Plugin Log Contains X Times   edr <> Run osquery process   2

    Sleep  60 secs
    #wait to ensure there is not second scheduled restart

    Run Keyword And Expect Error   *
    ...   Wait EDR Plugin Log Contains X Times  edr <> Restarting osquery  2
    Fail

*** Keywords ***
Check MTR in ALC Policy Forces Disable Audit Data Collection
    Check EDR Plugin Installed With Base
    ${alc} =  Get File  ${EXAMPLE_DATA_PATH}/ALC_policy_with_mtr_example.xml
    Install ALC Policy   ${alc}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Configured To Not Collect Audit Data

Check Running Instance of Osquery Configured To Collect Audit Data
    ${response} =  Run Live Query and Return Result
    Should Contain  ${response}  "columnMetaData": [{"name":"name","type":"TEXT"},{"name":"value","type":"TEXT"}]
    Should Contain  ${response}  "columnData": [["disable_audit","false"]]


Check Running Instance of Osquery Configured To Not Collect Audit Data
    ${response} =  Run Live Query and Return Result
    Should Contain  ${response}  "columnMetaData": [{"name":"name","type":"TEXT"},{"name":"value","type":"TEXT"}]
    Should Contain  ${response}  "columnData": [["disable_audit","true"]]


Run Live Query and Return Result
    [Arguments]  ${query}=SELECT name,value from osquery_flags where name == 'disable_audit'
    ${query_template} =  Get File  ${EXAMPLE_DATA_PATH}/GenericQuery.json
    ${query_json} =  Replace String  ${query_template}  %QUERY%  ${query}
    Log  ${query_json}
    Create File  ${EXAMPLE_DATA_PATH}/temp.json  ${query_json}
    ${response}=  Set Variable  ${SOPHOS_INSTALL}/base/mcs/response/LiveQuery_123-4_response.json
    Simulate Live Query  temp.json
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  File Should Exist    ${response}
    ${response} =  Get File  ${response}
    Remove File  ${response}
    Remove File  ${EXAMPLE_DATA_PATH}/temp.json
    [Return]  ${response}

Check Osquery Configured To Collect Audit Data
    ${osqueryConf} =   Get File  ${COMPONENT_ROOT_PATH}/etc/osquery.flags
    Should Contain  ${osqueryConf}  --disable_audit=false


Check Osquery Configured To Not Collect Audit Data
    ${osqueryConf} =   Get File  ${COMPONENT_ROOT_PATH}/etc/osquery.flags
    Should Contain  ${osqueryConf}  --disable_audit=true

Wait for EDR Log
    [Arguments]  ${log}
    Wait Until Keyword Succeeds
        ...  10 secs
        ...  1 secs
        ...  EDR Plugin Log Contains      ${log}

Wait EDR Plugin Log Contains X Times
    [Arguments]  ${log}  ${xtimes}
    Wait Until Keyword Succeeds
        ...  15 secs
        ...  1 secs
        ...  EDR Plugin Log Contains X Times  ${log}  ${xtimes}