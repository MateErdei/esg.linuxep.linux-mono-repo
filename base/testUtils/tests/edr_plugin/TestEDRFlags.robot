*** Settings ***
Suite Setup      EDR Suite Setup
Suite Teardown   EDR Suite Teardown

Test Setup       EDR Test Setup
Test Teardown    EDR Test Teardown

Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/LiveQueryUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py

Resource    ../GeneralTeardownResource.robot
Resource    EDRResources.robot
Resource    ../mdr_plugin/MDRResources.robot

Default Tags   EDR_PLUGIN  FAKE_CLOUD
*** Test Cases ***
Flags Are Only Sent To EDR and Not MTR
    Install MDR Directly
    Register With Fake Cloud
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  Check MCS Router Running
    Wait Until OSQuery Running  30

    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  Check Managementagent Log Contains  flags.json applied to 1 plugins

    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  Check EDR Log Contains  Applying new policy with APPID: FLAGS

    Check MTR Log Does Not Contain  Applying new policy with APPID: FLAGS
    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  Check EDR Log Contains  Flags running mode is EDR
    ${contents}=  Get File   ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${contents}   running_mode=1

EDR changes running mode when XDR enabled flags are sent
    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_xdr_enabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Should Be Equal As Strings  0  ${result.rc}
    Register With Fake Cloud
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  Check MCS Router Running
    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  Check Managementagent Log Contains  flags.json applied to 1 plugins
    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  Check EDR Log Contains  Flags running mode in policy is XDR
    Check EDR Log Contains  Updating XDR flag settings
    ${contents}=  Get File   ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${contents}   running_mode=0
    Should Not Contain  ${contents}   running_mode=1

    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  Check EDR Log Contains  Plugin Finished.

    Wait Until Keyword Succeeds
    ...  25
    ...  1
    ...  Check Log Contains In Order
            ...  ${SOPHOS_INSTALL}/logs/base/watchdog.log
            ...  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/edr/bin/edr exited
            ...  ProcessMonitoringImpl <> Starting /opt/sophos-spl/plugins/edr/bin/edr

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   5 secs
    ...   EDR Plugin Is Running

EDR runs sophos extension when XDR is enabled
    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_xdr_enabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Should Be Equal As Strings  0  ${result.rc}
    Register With Fake Cloud

    Wait Until Keyword Succeeds
    ...  45
    ...  5
    ...  Check EDR Log Contains  Starting SophosExtension

    Run Live Query  ${SOPHOS_INFO_QUERY}  sophos_info
    Wait Until Keyword Succeeds
    ...  50 secs
    ...  2 secs
    ...  Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log  Successfully executed query with name: sophos_info  1

    Check Log Contains String N times   ${SOPHOS_INSTALL}/plugins/edr/log/livequery.log   edr_log   "columnData": [["ThisIsAnMCSID+1001"]]  1

EDR disables curl tables when network available flag becomes false
    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_xdr_enabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Should Be Equal As Strings  0  ${result.rc}
    Register With Fake Cloud
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  Check MCS Router Running
    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  Check Managementagent Log Contains  flags.json applied to 1 plugins
    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  Check EDR Log Contains  Updating XDR flag settings
    ${contents}=  Get File   ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${contents}   network_tables=0
    Should Not Contain  ${contents}   network_tables=1

    Wait Until Keyword Succeeds
    ...  10
    ...  1
    ...  Check EDR Log Contains  Plugin Finished.

    Wait Until Keyword Succeeds
    ...  25
    ...  1
    ...  Check Log Contains In Order
            ...  ${SOPHOS_INSTALL}/logs/base/watchdog.log
            ...  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/edr/bin/edr exited
            ...  ProcessMonitoringImpl <> Starting /opt/sophos-spl/plugins/edr/bin/edr

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   5 secs
    ...   EDR Plugin Is Running

    Copy File  ${SUPPORT_FILES}/CentralXml/FLAGS_network_tables_disabled.json  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    ${result} =  Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json

    Wait Until Keyword Succeeds
    ...  60
    ...  5
    ...  Check EDR Log Contains  Table curl is disabled, not attaching

    ${contents}=  Get File   ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.flags
    Should Contain  ${contents}   --disable_tables=curl,curl_certificate

    ${contents}=  Get File   ${SOPHOS_INSTALL}/plugins/edr/etc/plugin.conf
    Should Contain  ${contents}   network_tables=1

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
