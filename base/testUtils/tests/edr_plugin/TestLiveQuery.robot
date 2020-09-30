*** Settings ***
Suite Setup      EDR Suite Setup
Suite Teardown   EDR Suite Teardown

Test Setup       EDR Test Setup
Test Teardown    EDR Test Teardown

Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py

Resource    ../GeneralTeardownResource.robot
Resource    EDRResources.robot

Default Tags   EDR_PLUGIN  FAKE_CLOUD
*** Test Cases ***
Install EDR And Get Historic Event Data
    Register With Fake Cloud
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  {"endpoint.flags1.enabled: "true", "endpoint.flags2.enabled: "force"}
    Wait Until OSQuery Running
    Sleep  5
    Fail
#    Wait Until Keyword Succeeds
#    ...  30
#    ...  1
#    ...  Check MCS Router Running
#    Wait Until OSQuery Running
#    # force process events to be generated
#    ${result}=  Run Process  python3  --version
#    sleep  1
#    ${result}=  Run Process  python3  --version
#    sleep  1
#    ${result}=  Run Process  python3  --version
#    sleep  1
#    ${result}=  Run Process  python3  --version
#    sleep  1
#    ${result}=  Run Process  python3  --version
#
#
#    Wait Until Keyword Succeeds
#    ...   7x
#    ...   10 secs
#    ...   Run Query Until It Gives Expected Results  select pid from process_events LIMIT 1  {"columnMetaData":[{"name":"pid","type":"BIGINT"}],"queryMetaData":{"errorCode":0,"errorMessage":"OK","rows":1}}

*** Keywords ***
EDR Test Setup
    Install EDR Directly
    Start Local Cloud Server   --initial-alc-policy    ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml

EDR Test Teardown
    Stop Local Cloud Server
    Run Keyword if Test Failed    Report Audit Link Ownership
    Run Keyword If Test Failed    Dump Cloud Server Log
    General Test Teardown
    Uninstall EDR Plugin
    Remove File   tmp/last_query_response.json

EDR Suite Setup
    Regenerate Certificates
    Require Fresh Install
    Set Local CA Environment Variable
    Override LogConf File as Global Level  DEBUG
    Check For Existing MCSRouter
    Cleanup MCSRouter Directories
    Cleanup Local Cloud Server Logs

EDR Suite Teardown
    Uninstall SSPL
    Unset CA Environment Variable
