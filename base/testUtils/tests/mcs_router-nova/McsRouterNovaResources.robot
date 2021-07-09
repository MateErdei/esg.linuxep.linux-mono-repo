*** Settings ***
Documentation    Shared keywords for MCS Router tests

Resource  ../mcs_router/McsRouterResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../mdr_plugin/MDRResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource  ../GeneralTeardownResource.robot

Library     ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/SystemInfo.py


*** Variables ***
${tmpdir}               ${SOPHOS_INSTALL}/tmp/SDT
${statusPath}           ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

*** Keywords ***

Dump Logs And Clean Up Temp Dir
    Run Keyword If Test Failed  Log File    /etc/hosts
    Stop Update Server
    Remove Directory   ${tmpdir}  recursive=true
    Reload Cloud Options


Setup Fresh Install Nova
    Require Fresh Install
    Set Nova MCS CA Environment Variable

    Generate Update Certs
    Remove Files  ${UPDATE_ROOTCERT_DIR}/ps_rootca.crt  ${UPDATE_ROOTCERT_DIR}/ps_rootca.crt.0  ${UPDATE_ROOTCERT_DIR}/rootca.crt   ${UPDATE_ROOTCERT_DIR}/rootca.crt.0  ${UPDATE_ROOTCERT_DIR}/cache_certificates.crt
    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${UPDATE_ROOTCERT_DIR}
    Copy File   ${SUPPORT_FILES}/sophos_certs/rootca.crt  ${UPDATE_ROOTCERT_DIR}
    Log File   /etc/hosts


Require Registered
    [Arguments]   ${waitForALCPolicy}=${False}
    Log To Console  ${regCommand}
    Register With Central  ${regCommand}
    Check MCS Router Running
    Wait For Server In Cloud
    Wait Until Keyword Succeeds
    ...  30
    ...  5
    ...  Check Update Scheduler Running

    Run Keyword If   ${waitForALCPolicy}==${True}
    ...  Check ALC Policy Exists

Wait until MCS policy is Downloaded
    Wait Until Keyword Succeeds
    ...  60
    ...  5 secs
    ...  Check Policy Files Exists  MCS-25_policy.xml

Wait until MDR policy is Downloaded
    Wait Until Keyword Succeeds
    ...  120
    ...  20 secs
    ...  Check Policy Files Exists  MDR_policy.xml

Check ALC Policy Exists
    Wait Until Keyword Succeeds
    ...  60
    ...  1
    ...  File Should Exist   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml


Check MCS Envelope Contains Event Success On N Event Sent
    [Arguments]  ${Event_Number}
    ${string}=  Check Log And Return Nth Occurence Between Strings   <event><appId>ALC</appId>  </event>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Event_Number}
    Should contain   ${string}   &lt;number&gt;0&lt;/number&gt;

Check MCS Envelope Contains Event Fail On N Event Sent
    [Arguments]  ${Event_Number}
    ${string}=  Check Log And Return Nth Occurence Between Strings   <event><appId>ALC</appId>  </event>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Event_Number}
    Should Not Contain   ${string}   &lt;number&gt;0&lt;/number&gt;

Register With Real Update Cache and Message Relay Account
    [Arguments]   ${messageRelayOptions}=
    Mark All Logs  Registering with Real Update Cache and Message Relay account
    # Remove the file so that we can use the "SulDownloader Reports Finished" function.
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Register With Central  ${regCommand} ${messageRelayOptions}


Nova Suite Teardown
    Dump Appserver Log
    Uninstall SSPL Unless Cleanup Disabled
    Revert Hostfile
    Run Keyword And Ignore Error   Log File  /tmp/registerCommand
    Remove File  /tmp/registerCommand

Nova Test Teardown
    [Arguments]  ${requireDeRegister}=False
    Run Keyword If Test Failed     Dump Appserver Log
    MCSRouter Test Teardown
    Dump Logs And Clean Up Temp Dir
    Run Keyword If  ${requireDeRegister}   Deregister From Central
    Run Keyword If Test Failed    Reset Environment For Nova Tests


Setup MCS Tests Nova
    Setup Host File
    Require Fresh Install
    Remove Environment Variable  MCS_CONFIG_SET
    Reload Cloud Options
    Set Nova MCS CA Environment Variable
    Get Credentials

Setup MCS Tests Nova Update cache
    Setup Host File
    Require Fresh Install
    Remove Environment Variable  MCS_CONFIG_SET
    Set Environment Variable  MCS_CONFIG_SET  ucmr-nova
    Reload Cloud Options
    Set Nova MCS CA Environment Variable
    Get Credentials

Get Credentials
    Wait Until Keyword Succeeds
    ...  120
    ...  30
    ...  Set Registration Command

Set Registration Command
    ${regCommand}=  Get Sspl Registration
    Set Suite Variable    ${regCommand}     ${regCommand}   children=true

Reset Environment For Nova Tests
    Nova Suite Teardown
    Setup Host File
    Unmount All Comms Component Folders
    Require Fresh Install
    Remove Environment Variable  MCS_CONFIG_SET
    Reload Cloud Options
    Set Nova MCS CA Environment Variable


