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

EDR Does Not Configure Audit If MTR Is Supposed To Be Installed
    Check MTR in ALC Policy Forces Disable Audit Data Collection

EDR configures osquery to collect audit data if MTR is removed
    Check MTR in ALC Policy Forces Disable Audit Data Collection
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Running Instance of Osquery Configured To Not Collect Audit Data

    ${alc} =  Get ALC Policy Without MTR
    Install ALC Policy   ${alc}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check Osquery Configured To Collect Audit Data

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Running Instance of Osquery Configured To Collect Audit Data


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
