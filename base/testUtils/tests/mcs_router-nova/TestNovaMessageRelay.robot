*** Settings ***
Documentation    Test SSPL via Message Relays with Nova Central

Resource  ../mcs_router/McsRouterResources.robot
Resource  McsRouterNovaResources.robot

Library     ${libs_directory}/CentralUtils.py

Default Tags  CENTRAL  MCS  EXCLUDE_AWS

Test Teardown     Real UCMR Test Teardown  requireDeRegister=True
Suite Teardown    Run Keywords
...               Require Fresh Install   AND
...               Set Nova MCS CA Environment Variable

*** Variables ***
${MESSAGE_RELAY_1_ID}  a29e6a89-c29e-4545-865f-a353f52030b5
${MESSAGE_RELAY_2_ID}  a0262abd-a3d4-4dfe-905a-c62bb55cac44
${MESSAGE_RELAY_1_HOSTNAME}  SSPLUC1
${MESSAGE_RELAY_2_HOSTNAME}  SSPLUC2
${MESSAGE_RELAY_1_HOSTNAME_LOWER}  sspluc1
${MESSAGE_RELAY_2_HOSTNAME_LOWER}  sspluc2
${MESSAGE_RELAY_1_PORT}  8190
${MESSAGE_RELAY_2_PORT}  8190


*** Test Cases ***
Check Registration Via Message Relay
    [Documentation]  Derived from CLOUD.PROXY.006_Register_in_cloud_through_message_relay.sh
    Register With Real Update Cache and Message Relay Account  --messagerelay=${MESSAGE_RELAY_1_HOSTNAME}:${MESSAGE_RELAY_1_PORT},0,${MESSAGE_RELAY_1_ID}
    Wait For Server In Cloud
    Check Register Central Log Contains In Order   Trying connection via message relay ${MESSAGE_RELAY_1_HOSTNAME}:${MESSAGE_RELAY_1_PORT}  Successfully connected to mcs.sandbox.sophos:443 via ${MESSAGE_RELAY_1_HOSTNAME}:${MESSAGE_RELAY_1_PORT}
    Check Register Central Log Contains In Order   <productType>sspl</productType>  <platform>linux</platform>  <isServer>1</isServer>

MCS Communicates With Nova Via Message Relay
    Register With Real Update Cache and Message Relay Account
    Wait For MCS Router To Be Running
    Check MCSRouter Log Contains  Successfully directly connected to mcs.sandbox.sophos:443
    Mark Log File  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log
    Wait New MCS Policy Downloaded
    Wait For Server In Cloud
    Check Marked Mcsrouter Log Contains   Successfully connected to mcs.sandbox.sophos:443 via ${MESSAGE_RELAY_1_HOSTNAME_LOWER}:${MESSAGE_RELAY_1_PORT}

Message Relays in MCS Policy Are Written into MCS Config File
    [Documentation]  Derived from CLOUD.MCS.007_mcs_handles_message_relay_policy.sh
    Register With Real Update Cache and Message Relay Account
    Wait For MCS Router To Be Running
    Wait New MCS Policy Downloaded
    Wait For Server In Cloud

    ${fileContent}  Get File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config
    Should Contain  ${fileContent}  message_relay_priority1=0  message_relay_port1=${MESSAGE_RELAY_1_PORT}  message_relay_address1=${MESSAGE_RELAY_1_HOSTNAME_LOWER}  message_relay_id1=${MESSAGE_RELAY_1_ID}
    Should Contain  ${fileContent}  message_relay_priority2=0  message_relay_port2=${MESSAGE_RELAY_2_PORT}  message_relay_address2=${MESSAGE_RELAY_2_HOSTNAME_LOWER}  message_relay_id2=${MESSAGE_RELAY_2_ID}

    ${fileContent}  Get File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    Should Contain  ${fileContent}  current_relay_id=${MESSAGE_RELAY_1_ID}

MCS Sends Status to Nova when Message Relay Changes
    [Documentation]  Derived from CLOUD.MCS.012
    Register With Real Update Cache and Message Relay Account
    Wait For MCS Router To Be Running
    Check MCSRouter Log Contains  Successfully directly connected to mcs.sandbox.sophos:443
    Mark Log File  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log
    Mark MCS Envelope Log
    Wait New MCS Policy Downloaded
    Wait For Server In Cloud

    Wait Until Keyword Succeeds
    ...  180 secs
    ...  20 secs
    ...  Check Marked Mcsrouter Log Contains   Successfully connected to mcs.sandbox.sophos:443 via ${MESSAGE_RELAY_1_HOSTNAME_LOWER}:${MESSAGE_RELAY_1_PORT}
    Wait Until Keyword Succeeds
    ...  180 secs
    ...  20 secs
    ...  Check Marked MCS Envelope Log Contains  PUT /statuses/endpoint
    Check Marked MCS Envelope Log Contains  messageRelay endpointId=&amp;quot;${MESSAGE_RELAY_1_ID}

    Block Traffic to Message Relay  ${MESSAGE_RELAY_1_HOSTNAME_LOWER}
    Mark MCS Envelope Log
    Wait Until Keyword Succeeds
    ...  180 secs
    ...  20 secs
    ...  Check Marked Mcsrouter Log Contains   Successfully connected to mcs.sandbox.sophos:443 via ${MESSAGE_RELAY_2_HOSTNAME_LOWER}:${MESSAGE_RELAY_2_PORT}
    Wait Until Keyword Succeeds
    ...  180 secs
    ...  20 secs
    ...  Check Marked MCS Envelope Log Contains  messageRelay endpointId=&amp;quot;${MESSAGE_RELAY_2_ID}


*** Keywords ***
Real UCMR Test Teardown
    [Arguments]  ${requireDeRegister}=False
    Unblock Traffic to Message Relay  sspluc1
    Run Keyword If Test Failed     Dump Appserver Log
    MCSRouter Test Teardown
    Dump Logs And Clean Up Temp Dir
    Run Keyword If  ${requireDeRegister}   Deregister From Central
    Remove Environment Variable  MCS_CONFIG_SET
    Reload Cloud Options
    Reset Environment For Nova Tests