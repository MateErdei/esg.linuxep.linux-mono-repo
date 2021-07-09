*** Settings ***
Documentation    Test SSPL via Message Relays with Nova Central

Resource  ../mcs_router/McsRouterResources.robot
Resource  McsRouterNovaResources.robot

Library     ${LIBS_DIRECTORY}/CentralUtils.py

Default Tags  CENTRAL  MCS  EXCLUDE_AWS
Suite Setup  Setup MCS Tests Nova Update cache
Test Teardown     Real UCMR Test Teardown  requireDeRegister=True


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
Check Registration and connection Via Message Relay
    [Documentation]  Derived from CLOUD.PROXY.006_Register_in_cloud_through_message_relay.sh
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  updatescheduler
    Remove File  ${SOPHOS_INSTALL}/base/bin/UpdateScheduler
    Register With Real Update Cache and Message Relay Account  --messagerelay=${MESSAGE_RELAY_1_HOSTNAME}:${MESSAGE_RELAY_1_PORT},0,${MESSAGE_RELAY_1_ID}
    Wait For Server In Cloud
    Check Register Central Log Contains In Order   Trying connection via message relay ${MESSAGE_RELAY_1_HOSTNAME}:${MESSAGE_RELAY_1_PORT}  Successfully connected to mcs.sandbox.sophos:443 via ${MESSAGE_RELAY_1_HOSTNAME}:${MESSAGE_RELAY_1_PORT}
    Check Register Central Log Contains In Order   <productType>sspl</productType>  <platform>linux</platform>  <isServer>1</isServer>
    Check MCSRouter Log Contains  Successfully directly connected to mcs.sandbox.sophos:443
    Mark Log File  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log
    Wait New MCS Policy Downloaded  60
    Wait For Server In Cloud
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  10 secs
    ...  Check Marked Mcsrouter Log Contains   Successfully connected to mcs.sandbox.sophos:443 via sspluc



*** Keywords ***
Real UCMR Test Teardown
    [Arguments]  ${requireDeRegister}=False
    Unblock Traffic to Message Relay  sspluc1
    Run Keyword If Test Failed     Dump Appserver Log
    MCSRouter Test Teardown
    Dump Logs And Clean Up Temp Dir
    Run Keyword If  ${requireDeRegister}   Deregister From Central