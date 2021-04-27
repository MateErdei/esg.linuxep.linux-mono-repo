*** Settings ***
Documentation    Tests to verify we can register successfully with
...              fake cloud and save the ID and password we receive.
...              Also tests bad registrations, and deregistrations.

Library     OperatingSystem
Library     ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/UpdateServer.py
Library     ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py   # for replace_sophos_urls_to_localhost
Library     String

Resource  ../management_agent-event_processor/EventProcessorResources.robot
Resource  ../installer/InstallerResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource  ../mcs_router/McsRouterResources.robot
Resource  ../mcs_router-nova/McsRouterNovaResources.robot
Resource  ../installer/InstallerResources.robot
Resource  ../upgrade_product/UpgradeResources.robot

Test Setup     Local Test Setup
Test Teardown  Local Test Teardown

Suite Setup    Local Suite Setup
Suite Teardown  Local Suite Teardown

Default Tags  MCS  FAKE_CLOUD  UPDATE_SCHEDULER  MCS_ROUTER  OSTIA
Force Tags  LOAD4

*** Variables ***
${tmpdir}                       ${SOPHOS_INSTALL}/tmp/SDT
${BASE_VUT_POLICY}   ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_only_VUT.xml
${BASE_AND_MDR_VUT_POLICY}   ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_mtr_VUT.xml
${statusPath}  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml


*** Test Case ***
Verify Status Message Is Sent When New Policy Received Even If Product Update Is Not Executed
    Start Local Cloud Server  --initial-alc-policy  ${BASE_VUT_POLICY}
    Prepare Installation For Upgrade Using Policy  ${BASE_VUT_POLICY}
    Set Local CA Environment Variable
    Register With Local Cloud Server

    Check Correct MCS Password And ID For Local Cloud Saved

    #Set policy being sent from cloud before starting MCS
    ${scheduled_update_policy}=  Set Variable  ${SUPPORT_FILES}/CentralXml/ALC_policy_scheduled_update.xml

    Start System Watchdog

    Wait Until Keyword Succeeds
    ...  2 min
    ...  10 secs
    ...  Check MCSenvelope Log Contains  ThisIsAnMCSID+1001

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BASE_VUT_POLICY}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Exist  ${UPDATE_CONFIG}

    #Status is based upon the products on the system and as such only changes with policy

    # wait for initial update to succeed
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    #New status
    Should Exist    ${MCS_DIR}/status/ALC_status.xml
    #Remove status file and expect that it will not be regenerated until a change in policy
    Remove File  ${MCS_DIR}/status/ALC_status.xml

    Send Policy File  alc  ${scheduled_update_policy}

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${scheduled_update_policy}

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   Should Exist    ${MCS_DIR}/status/ALC_status.xml




*** Keywords ***
Check MCS Envelope for Status Being Same On N Status Sent
    [Arguments]  ${Status_Number}
    ${string}=  Check Log And Return Nth Occurence Between Strings   <status><appId>ALC</appId>  </status>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Status_Number}
    Should contain   ${string}   Res=&amp;quot;Same&amp;quot;

Change Update Certs In Installed Base To Use Locally Generated Certs
    Copy File   ${SUPPORT_FILES}/sophos_certs/rootca.crt  ${UPDATE_ROOTCERT_DIR}/rootca.crt
    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${UPDATE_ROOTCERT_DIR}/ps_rootca.crt

Wait For Update Status File
    Wait Until Keyword Succeeds
    ...  2 min
    ...  2 secs
    ...  Should Exist    ${MCS_DIR}/status/ALC_status.xml
    Log File  ${MCS_DIR}/status/ALC_status.xml

Create Warehouse From Dist
    ${dist} =  Get Folder With Installer
    Copy Directory  ${dist}  ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Base

    Copy File   ${SUPPORT_FILES}/sophos_certs/rootca.crt  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/update/rootcerts/rootca.crt
    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/update/rootcerts/ps_rootca.crt
    Remove File  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/etc/logger.conf
    #Turn on debug logging on update
    Create File  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n

    Add Component Warehouse Config   ServerProtectionLinux-Base   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ServerProtectionLinux-Base
    Generate Warehouse

    Start Update Server    1233    ${tmpdir}/temp_warehouse//customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1234/catalogue/sdds.ServerProtectionLinux-Base.xml
    Can Curl Url    https://localhost:1233
    Set Local CA Environment Variable


Create Warehouse With Base And Example Plugin
    ${PathToBase} =  Get Folder With Installer
    Remove Directory  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}  recursive=${TRUE}
    Copy Directory     ${PathToBase}  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}

    Copy File   ${SUPPORT_FILES}/sophos_certs/rootca.crt  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/update/rootcerts/rootca.crt
    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/update/rootcerts/ps_rootca.crt
    Remove File  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/etc/logger.conf
    #Turn on debug logging on update
    Create File  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/etc/logger.conf  [global]\nVERBOSITY = DEBUG\n

    ${PathToExamplePlugin} =  Get Exampleplugin Sdds
    Remove Directory  ${tmpdir}/TestInstallFiles/${EXAMPLE_PLUGIN_RIGID_NAME}  recursive=${TRUE}
    Copy Directory     ${PathToExamplePlugin}  ${tmpdir}/TestInstallFiles/${EXAMPLE_PLUGIN_RIGID_NAME}

    Clear Warehouse Config
    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}  ${BASE_RIGID_NAME}
    Add Component Warehouse Config   ${EXAMPLE_PLUGIN_RIGID_NAME}  ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${EXAMPLE_PLUGIN_RIGID_NAME}  ${BASE_RIGID_NAME}

    Generate Warehouse

    Start Update Server    1233    ${tmpdir}/temp_warehouse/customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1234/catalogue/sdds.live.xml
    Can Curl Url    https://localhost:1233
    Set Local CA Environment Variable


Wait Until Update Event Has Been Sent
    Wait Until Keyword Succeeds
    ...  1 min
    ...  2 secs
    ...  Check MCSenvelope Log Contains   <event><appId>ALC</appId>

Wait Until ALC Status Has Been Sent
    Wait Until Keyword Succeeds
    ...  1 min
    ...  2 secs
    ...  Check MCSenvelope Log Contains   <status><appId>ALC</appId>


Local Test Setup
    Require Fresh Install
    Stop System Watchdog
    Generate Update Certs

Local Test Teardown
    Log SystemCtl Update Status
    MCSRouter Default Test Teardown
    Stop Update Server
    Stop Local Cloud Server

Local Suite Setup
    Generate Update Certs
    Setup Ostia Warehouse Environment

Local Suite Teardown
    Teardown Ostia Warehouse Environment