*** Settings ***
Suite Setup      EDR Suite Setup
Suite Teardown   EDR Suite Teardown

Test Setup       EDR Test Setup
Test Teardown    EDR Test Teardown

Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/LiveQueryUtils.py
Library     ${COMMON_TEST_LIBS}/MCSRouter.py

Resource    ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/GeneralUtilsResources.robot

Default Tags   EDR_PLUGIN  FAKE_CLOUD
Force Tags  TAP_PARALLEL3

*** Test Cases ***
EDR disables curl tables when network available flag becomes false
    Override Local LogConf File for a component   DEBUG  global

    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_network_tables_enabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Should Be Equal As Strings  0  ${result.rc}
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  Check MCS Router Running
    Wait Until Keyword Succeeds
    ...  60
    ...  5
    ...  File Should Contain    ${SOPHOS_INSTALL}/base/mcs/policy/flags.json     "livequery.network-tables.available": true

    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  Check EDR Log Contains  Updating network_tables flag settings
    ${contents}=  Get File   ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${contents}   network_tables=1
    Should Not Contain  ${contents}   network_tables=0

    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  Check EDR Log Contains  osquery stopped. Scheduling its restart in 0 seconds.

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   5 secs
    ...   Check EDR Osquery Executable Running

    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  File Should Not Contain  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.flags  --disable_tables=curl,curl_certificate
    #mark edr osquery log because "Table curl is disabled" can appear before the test starts on a slow system
    Mark EDR Osquery Log
    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_network_tables_disabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json

    Wait Until Keyword Succeeds
    ...  60
    ...  5
    ...  Check Marked Edr OSQuery Log Contains  Table curl is disabled, not attaching

    Wait Until Keyword Succeeds
    ...  10
    ...  2
    ...  File Should Contain  ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.flags  --disable_tables=curl,curl_certificate

    ${contents}=  Get File   ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${contents}   network_tables=0

*** Keywords ***

EDR Test Setup
    Start Local Cloud Server   --initial-alc-policy    ${SUPPORT_FILES}/CentralXml/ALC_policy_direct.xml
    Register With Fake Cloud
    Install EDR Directly


EDR Test Teardown
    Stop Local Cloud Server
    Run Keyword if Test Failed    Report Audit Link Ownership
    Run Keyword If Test Failed    Dump Cloud Server Log
    MCSRouter Default Test Teardown
    Uninstall EDR Plugin

EDR Suite Setup
    Regenerate Certificates
    Require Fresh Install
    Set Local CA Environment Variable
    Override LogConf File as Global Level  DEBUG
    Run Process   systemctl  restart  sophos-spl
    Wait For Base Processes To Be Running
    Check For Existing MCSRouter
    Cleanup MCSRouter Directories
    Cleanup Local Cloud Server Logs

EDR Suite Teardown
    Uninstall SSPL
    Unset CA Environment Variable
