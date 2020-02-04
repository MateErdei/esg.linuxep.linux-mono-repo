*** Settings ***

Library    DateTime

Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/UpdateServer.py

Library    ${LIBS_DIRECTORY}/Watchdog.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/MDRUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  ../GeneralTeardownResource.robot
Resource  ../installer/InstallerResources.robot
Resource  ../mcs_router/McsRouterResources.robot
Resource  MDRResources.robot

Suite Setup      Setup MTR Message Relay Updating Suite
Suite Teardown   Teardown MTR Message Relay Updating Suite

Test Setup       Setup MTR Message Relay Updating Test
Test Teardown    Teardown MTR Message Relay Updating Test

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER  MDR_PLUGIN  REGISTRATION




*** Keywords ***

Setup MTR Message Relay Updating Suite
    Regenerate Certificates
    Setup MCS Tests


Teardown MTR Message Relay Updating Suite
    Cleanup Certificates


Setup MTR Message Relay Updating Test
    Start Local Cloud Server

    Block Connection Between EndPoint And FleetManager
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY=DEBUG\n


Teardown MTR Message Relay Updating Test
    Run Keyword If Test Failed  Dump Log   ${MTR_MESSAGE_RELAY_CONFIG_FILE}

    Stop Local Cloud Server
    MCSRouter Default Test Teardown   # Includes the general test teardown

    Run Keyword And Ignore Error  Remove File  ${MDRBasePolicy}
    Restore Connection Between EndPoint and FleetManager
    Run Keyword And Ignore Error  Uninstall MDR Plugin
    Check MDR Plugin Uninstalled

    Stop System Watchdog


Wait Message Relay Structure Is Correct And Contains Port
    [Arguments]    ${PORT}    ${MCSID}=1001
    Wait Until Keyword Succeeds
    ...  1 min
    ...  20 secs
    ...  Check Message Relay Structure Is Correct And Contains Port    ${PORT}   ${MCSID}

Check Message Relay Structure Is Correct And Contains Port
    [Arguments]    ${PORT}    ${MCSID}=1001
    Check File Exists   ${MTR_MESSAGE_RELAY_CONFIG_FILE}

    ${FileList}=  Create List  ${MCS_POLICY_CONFIG}  ${MCS_CONFIG}
    ${MOST_RECENT_EDIT_EPOCH_TIME}=  Find Most Recent Edit Time From List Of Files  ${FileList}

    ${MOST_RECENT_EDIT_ISO_8601_TIME} =  Convert Epoch Time To UTC ISO 8601  ${MOST_RECENT_EDIT_EPOCH_TIME}

    Check Log Contains    <selectedMessageRelay address="localhost:${PORT}" lastUsed="${MOST_RECENT_EDIT_ISO_8601_TIME}" id="ThisIsAnMCSID+${MCSID}"/>   ${MTR_MESSAGE_RELAY_CONFIG_FILE}    MessageRelayConfig.xml


*** Test Cases ***

MTR Has No Message Relay Configured If Not Defined In MCS Config Files
    Register With Local Cloud Server

    Start Watchdog

    Check Cloud Server Log For Command Poll

    Install MDR Directly
    Check MDR Plugin Installed

    Check Log Does Not Contain    selectedMessageRelay    ${MTR_MESSAGE_RELAY_CONFIG_FILE}    MessageRelayConfig.xml


MTR Gets Message Relay Information From MCS Config Files Created By MCS Router
    Start Simple Proxy Server    3000
    Register With Local Cloud Server

    Start Watchdog

    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='3000' address='localhost' id='no_auth_proxy'/>

    Check Cloud Server Log For Command Poll

    Install MDR Directly
    Check MDR Plugin Installed

    Wait Message Relay Structure Is Correct And Contains Port    3000


MTR Updates Message Relay Information When MCS Router Changes Message Relay
    ${PROXY_PORT_ONE} =  Set Variable  3331
    ${PROXY_PORT_TWO} =  Set Variable  7771

    Start Simple Proxy Server    ${PROXY_PORT_ONE}
    Start Simple Proxy Server    ${PROXY_PORT_TWO}
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='${PROXY_PORT_ONE}' address='localhost' id='Proxy_Name_One'/><messageRelay priority='0' port='${PROXY_PORT_TWO}' address='localhost' id='Proxy_Name_Two'/>

    Start Watchdog

    Install MDR Directly
    Check MDR Plugin Installed

    Check Cloud Server Log For Command Poll

    Wait Message Relay Structure Is Correct And Contains Port    ${PROXY_PORT_ONE}

    Stop Proxy Server On Port  ${PROXY_PORT_ONE}
    Check Cloud Server Log For Command Poll  2

    Wait Message Relay Structure Is Correct And Contains Port    ${PROXY_PORT_TWO}


MTR Removes Message Relay Information When MCS Router Connects Directly
    Start Simple Proxy Server    3000
    Register With Local Cloud Server

    Start Watchdog

    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='3000' address='localhost' id='no_auth_proxy'/>

    Install MDR Directly
    Check MDR Plugin Installed

    Check Cloud Server Log For Command Poll

    Wait Message Relay Structure Is Correct And Contains Port    3000

    Stop Proxy Server On Port  3000
    Check Cloud Server Log For Command Poll  2

    Check Log Does Not Contain    selectedMessageRelay    ${MTR_MESSAGE_RELAY_CONFIG_FILE}    MessageRelayConfig.xml


MTR Adds New Message Relay Information From MCS Config Files
    Start Simple Proxy Server    3000
    Register With Local Cloud Server

    Start Watchdog

    Check Cloud Server Log For Command Poll

    Install MDR Directly
    Check MDR Plugin Installed

    Check Log Does Not Contain    selectedMessageRelay    ${MTR_MESSAGE_RELAY_CONFIG_FILE}    MessageRelayConfig.xml

    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='3000' address='localhost' id='no_auth_proxy'/>

    #Message relays are evaluated and written to config on command poll after the policy is received
    Check Cloud Server Log For Command Poll  3

    Wait Message Relay Structure Is Correct And Contains Port    3000


MTR Removes Message Relay When Deregistering And Adds It When Reregistering
    Start Simple Proxy Server    3000
    Register With Local Cloud Server

    Start Watchdog

    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='3000' address='localhost' id='no_auth_proxy'/>
    Check Cloud Server Log For Command Poll

    Install MDR Directly
    Check MDR Plugin Installed

    Wait Message Relay Structure Is Correct And Contains Port    3000

    Deregister From Central

    Check Log Does Not Contain    selectedMessageRelay    ${MTR_MESSAGE_RELAY_CONFIG_FILE}    MessageRelayConfig.xml

    Register With Local Cloud Server

    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='3000' address='localhost' id='no_auth_proxy'/>

    Wait Message Relay Structure Is Correct And Contains Port    3000    1002



MTR Fails To Add Message Relay Information When MCS Config Files Are Missing But Recovers
    Start Simple Proxy Server    3000

    Start Watchdog

    Install MDR Directly
    Check MDR Plugin Installed

    Check Log Does Not Contain    selectedMessageRelay    ${MTR_MESSAGE_RELAY_CONFIG_FILE}    MessageRelayConfig.xml

    ${EXPECTED_ERROR}=     Set Variable    Failed to read mcs config file(s): ${MCS_CONFIG} ${MCS_POLICY_CONFIG}
    Check Log Contains    ${EXPECTED_ERROR}    ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log    mtr.log

    Register With Local Cloud Server

    Send Mcs Policy With New Message Relay   <messageRelay priority='0' port='3000' address='localhost' id='no_auth_proxy'/>

    Wait Message Relay Structure Is Correct And Contains Port    3000

