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

*** Variables ***
${tmpdir}                       ${SOPHOS_INSTALL}/tmp/SDT
${BASE_VUT_POLICY}   ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_only_VUT.xml
${BASE_AND_MDR_VUT_POLICY}   ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_and_mtr_VUT.xml
${sulConfigPath}  ${SOPHOS_INSTALL}/base/update/var/update_config.json
${statusPath}  ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml


*** Test Case ***
Verify Status Message Sent When Registering With Central
    Start Local Cloud Server  --initial-alc-policy  ${BASE_VUT_POLICY}
    Prepare Installation For Upgrade Using Policy  ${BASE_VUT_POLICY}
    Set Local CA Environment Variable
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Start System Watchdog

    Wait Until Keyword Succeeds
    ...  2 min
    ...  2 secs
    ...  Check MCSenvelope Log Contains  ThisIsAnMCSID+1001

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BASE_VUT_POLICY}

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Exist  ${sulConfigPath}

    @{policyValues} =  Get Alc Policy File Info  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    @{configValues} =  Get Sul Config File Info  ${sulConfigPath}
    Lists Should Be Equal  ${policyValues}  ${configValues}

    Log File  ${MCS_DIR}/status/ALC_status.xml
    Remove File  ${MCS_DIR}/status/ALC_status.xml

    # We want an update event sent up to central regardless of if the product actualy updates any
    # files it should send an event whenever it does the first update after a registration.

    # wait for initial update to succeed
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    Remove File  ${MCS_DIR}/status/ALC_status.xml

    Wait Until Update Event Has Been Sent

    Log File  ${BASE_LOGS_DIR}/sophosspl/mcs_envelope.log
    Remove File  ${BASE_LOGS_DIR}/sophosspl/mcs_envelope.log

    #Register again and wait for ThisIsAnMCSID+1002 in MCS logs, showing we've registered again.
    List Files In Directory 	${UPDATE_DIR}/var 	report*.json
    Register With New Token Local Cloud Server
    List Files In Directory 	${UPDATE_DIR}/var 	report*.json

    #Wait until the following message appears in the mcs envelope as it marks when the registration is
    #complete and all plugins have policies.
    Wait Until Keyword Succeeds
    ...  2 min
    ...  10 secs
    ...  Check MCSenvelope Log Contains  PUT /statuses/endpoint/ThisIsAnMCSID+1002

    # Force an update so that we don't have to wait 5 to 10 mins
    Trigger Update Now

    Wait For Update Status File

    Wait Until Update Event Has Been Sent


Verify Status Message Sent When Registering With Central And Event And Status Sent On Reregister
    Start Local Cloud Server  --initial-alc-policy  ${BASE_VUT_POLICY}
    Prepare Installation For Upgrade Using Policy  ${BASE_VUT_POLICY}
    Set Local CA Environment Variable
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Start System Watchdog

    Wait Until Keyword Succeeds
    ...  2 min
    ...  2 secs
    ...  Check MCSenvelope Log Contains  ThisIsAnMCSID+1001

    #When mcs router starts it will get the initial alc policy which in this case will contain valid credentials

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Exist  ${sulConfigPath}

    Log File  ${sulConfigPath}

    #First ALC policy will trigger update
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent   1

    # First two statuses sent up will be NoRef generated when update schedular starts for first time and when
    # MCS router restarts after first update.
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Mcs Envelope For Status Same In At Least One Of The Following Statuses Sent With The Prior Being Noref  2  3
    # call it again to capture the status we found same in, because robot is awkward
    ${statusWithResultSame} =  Check Mcs Envelope For Status Same In At Least One Of The Following Statuses Sent With The Prior Being Noref  2  3
    # add one to get the status we expect the next result=same to be in
    ${StatusToCheckForNextResultSame} =  Evaluate  ${statusWithResultSame}+${1}

    Prepare Installation For Upgrade Using Policy  ${BASE_VUT_POLICY}
    Register With New Token Local Cloud Server

    #Wait until the following message appears in the mcs envelope as it marks when the registration is
    #complete and all plugins have policies.
    Wait Until Keyword Succeeds
    ...  2 min
    ...  10 secs
    ...  Check MCSenvelope Log Contains  PUT /statuses/endpoint/ThisIsAnMCSID+1002

    #On reregister the alc policy will be sent again this will then trigger update scheduler to do an update
    #and, although nothing has changed on the box, cause an event and status is sent to central after the reregister

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent   2

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check MCS Envelope for Status Being Same On N Status Sent  ${StatusToCheckForNextResultSame}


Verify Status Message And Event Is Sent On First Update And Not On Following Updates If Product Doesnt Change
    [Tags]  MCS  FAKE_CLOUD  UPDATE_SCHEDULER  MCS_ROUTER  OSTIA
    ${defaultpolicy}=  Set Variable   ${SUPPORT_FILES}/CentralXml/ALC_policy_for_upgrade_test_just_base.xml
    ${statusPath}=  Set Variable  ${MCS_DIR}/status/ALC_status.xml
    ${UpdateSchedulerLog} =  Set Variable  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log

    Start Local Cloud Server  --initial-alc-policy  ${BASE_VUT_POLICY}
    Prepare Installation For Upgrade Using Policy  ${BASE_VUT_POLICY}
    Set Local CA Environment Variable
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Override LogConf File as Global Level  DEBUG
    Start System Watchdog

    Wait Until Keyword Succeeds
    ...  2 min
    ...  10 secs
    ...  Check MCSenvelope Log Contains  ThisIsAnMCSID+1001

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BASE_VUT_POLICY}

    #Update occurs on receiving first policy

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    Check Log Contains String N Times   ${UpdateSchedulerLog}  Update Log  Sending event to Central  1
    #Send status multiple times due to receiving policy, restarting after update and running suldownloader
    Check Log Contains String N Times   ${UpdateSchedulerLog}  Update Log  SulDownloader Finished.  1

    #Status is based upon the products on the system and as such only changes with policy
    Should Exist    ${statusPath}
    #Remove status file and expect that it will not be regenerated until successful update
    Remove File  ${statusPath}
    Should Not Exist    ${statusPath}

    # Trigger successful update and expect no event and no status as nothing has changed.
    Prepare Installation For Upgrade Using Policy  ${BASE_VUT_POLICY}
    Trigger Update Now

    ${UpToDateMessage}=  Set Variable  Downloaded Product line: 'ServerProtectionLinux-Base-component' is up to date.

    Wait Until Keyword Succeeds
    ...   70 secs
    ...   5 secs
    ...   Check Suldownloader Log Contains   ${UpToDateMessage}

    #Check that no event is sent after suldownloader finishes
    Wait Until Keyword Succeeds
    ...   20 secs
    ...   2 secs
    ...   Check Log Contains String N Times   ${UpdateSchedulerLog}  Update Log  SulDownloader Finished.  2

    #Put in slight delay to avoid potential race condition whereby this test would pass
    #when it was sending a second event
    Sleep  0.1

    #Only one event sent after two updates.
    Check Log Contains String N Times   ${UpdateSchedulerLog}  Update Log  Sending event to Central  1

    #Status cache means that the status will not be written again
    Should Not Exist    ${statusPath}

Verify Failure Event Is Sent on Update Failed
    [Tags]  MCS  FAKE_CLOUD  UPDATE_SCHEDULER  MCS_ROUTER  OSTIA
    Start Local Cloud Server  --initial-alc-policy  ${BASE_VUT_POLICY}

    Set Local CA Environment Variable
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

    Override LogConf File as Global Level  DEBUG
    Start System Watchdog

    Wait Until Keyword Succeeds
    ...  2 min
    ...  10 secs
    ...  Check MCSenvelope Log Contains  ThisIsAnMCSID+1001

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BASE_VUT_POLICY}

    #Update occurs on receiving first policy
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Fail On N Event Sent  1


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
    ...  File Should Exist  ${sulConfigPath}

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

Verify Status Message Is Sent On First Successful Update And On A Following Update Where Product Changes
    [Tags]  MCS  MCS_ROUTER  EXAMPLE_PLUGIN  FAKE_CLOUD  UPDATE_SCHEDULER  OSTIA
    ${statusPath}=  Set Variable  ${MCS_DIR}/status/ALC_status.xml

    Start Local Cloud Server  --initial-alc-policy  ${BASE_VUT_POLICY}
    Prepare Installation For Upgrade Using Policy  ${BASE_VUT_POLICY}
    Set Local CA Environment Variable
    Register With Local Cloud Server

    Check Correct MCS Password And ID For Local Cloud Saved

    Start System Watchdog
    Wait Until Keyword Succeeds
    ...  2 min
    ...  10 secs
    ...  Check MCSenvelope Log Contains  ThisIsAnMCSID+1001

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BASE_VUT_POLICY}

    #First ALC policy received will trigger an update

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   5 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    #New status
    Should Exist    ${statusPath}
    #Remove status file and expect that it will not be regenerated until a change in policy
    Remove File  ${statusPath}
    Should Not Exist    ${statusPath}

    #Sending new policy via fake cloud to cause MTR plugin to be installed
    Send Policy File  alc  ${BASE_AND_MDR_VUT_POLICY}
    Prepare Installation For Upgrade Using Policy  ${BASE_AND_MDR_VUT_POLICY}

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BASE_AND_MDR_VUT_POLICY}

    Prepare Installation For Upgrade Using Policy  ${BASE_VUT_POLICY}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   5 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  2

    #A status should exist as example plugin has been installed
    Should Exist    ${statusPath}



*** Keywords ***

Check MCS Envelope for Status Being Same On N Status Sent
    [Arguments]  ${Status_Number}
    ${string}=  Check Log And Return Nth Occurence Between Strings   <status><appId>ALC</appId>  </status>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Status_Number}
    Should contain   ${string}   Res=&amp;quot;Same&amp;quot;

Change Update Certs In Installed Base To Use Locally Generated Certs
    Copy File   ${SUPPORT_FILES}/sophos_certs/rootca.crt  ${SOPHOS_INSTALL}/base/update/certs/rootca.crt
    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${SOPHOS_INSTALL}/base/update/certs/ps_rootca.crt

Wait For Update Status File
    Wait Until Keyword Succeeds
    ...  2 min
    ...  2 secs
    ...  Should Exist    ${MCS_DIR}/status/ALC_status.xml
    Log File  ${MCS_DIR}/status/ALC_status.xml

Create Warehouse From Dist
    ${dist} =  Get Folder With Installer
    Copy Directory  ${dist}  ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Base

    Copy File   ${SUPPORT_FILES}/sophos_certs/rootca.crt  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/update/certs/rootca.crt
    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/update/certs/ps_rootca.crt
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

    Copy File   ${SUPPORT_FILES}/sophos_certs/rootca.crt  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/update/certs/rootca.crt
    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/files/base/update/certs/ps_rootca.crt
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