*** Settings ***

Test Setup       Require Uninstalled
Test Teardown    Test Teardown

Library     ${LIBS_DIRECTORY}/MTRService.py

Resource    EDRResources.robot

Default Tags   EDR_PLUGIN   MDR_PLUGIN

*** Test Cases ***
EDR takes netlink once MTR stops running scheduled queries
    # Start fake MTR backend
    Start MTRService

    Run Full Installer
    Override LogConf File as Global Level  DEBUG
    Create File  /opt/sophos-spl/base/etc/sophosspl/mcs.config  content=MCSToken=ThisIsARegToken \n MCSURL=https://localhost:4443/mcs \n current_relay_id=None \n MCSID=DummyEndpointID
    Create File  /opt/sophos-spl/base/etc/sophosspl/mcs_policy.config  content=customerId=DummyCustomerID

    Install EDR Directly
    Wait For EDR to be Installed
    Install Functional MTR From Fake Component Suite

    Copy File  ${GeneratedWarehousePolicies}/base_edr_and_mtr.xml             ${SOPHOS_INSTALL}/base/mcs/tmp/ALC-1_policy.xml
    Move File  ${SOPHOS_INSTALL}/base/mcs/tmp/ALC-1_policy.xml                  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

    # Connect MTR to fake MTR Service backend
    fix_dbos_certs_and_restart
    Check Osquery Is Sending Scheduled Query Results
    Check Osquery Respond To LiveQuery
    Check Netlink Owned By MTR

    # Send config without scheduled queries
    Swap Dbos Config and Restart MTR Plugin

    Wait Until Keyword Succeeds
    ...   100 secs
    ...   5 secs
    ...   Check Netlink Owned By EDR
    Swap Dbos Config Back and Restart MTR Plugin

    Wait Until Keyword Succeeds
    ...   100 secs
    ...   5 secs
    ...   Check Netlink Owned By MTR

*** Keywords ***
Check Netlink Owned By EDR
    ${NetlinkOwner} =  Report Audit Link Ownership
    should contain  ${NetlinkOwner}  edr
    should not contain  ${NetlinkOwner}  mtr

Check Netlink Owned By MTR
    ${NetlinkOwner} =  Report Audit Link Ownership
    should contain  ${NetlinkOwner}  mtr
    should not contain  ${NetlinkOwner}  edr

Test Teardown
    Stop MTRService
    Run Keyword And Ignore Error  Move File  /tmp/test/mock_darkbytes/config.json.original  /tmp/test/mock_darkbytes/config.json
    Run Keyword If Test Failed   Dump MTR Service Logs
    Run Keyword if Test Failed    Report Audit Link Ownership
    General Test Teardown

Swap Dbos Config and Restart MTR Plugin
    Stop Only MDR Plugin
    Move File  /tmp/test/mock_darkbytes/config.json  /tmp/test/mock_darkbytes/config.json.original
    Copy File  ${SUPPORT_FILES}/mock_darkbytes/configNoScheduledQueries.json  /tmp/test/mock_darkbytes/config.json
    Start MDR Plugin
    Wait Until SophosMTR Executable Running

Swap Dbos Config Back and Restart MTR Plugin
    Stop Only MDR Plugin
    Move File  /tmp/test/mock_darkbytes/config.json.original  /tmp/test/mock_darkbytes/config.json
    Start MDR Plugin
    Wait Until SophosMTR Executable Running