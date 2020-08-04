*** Settings ***
Documentation    Check MCS handles updates via Nova Central

Resource  ../mcs_router/McsRouterResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  McsRouterNovaResources.robot
Resource  ../mdr_plugin/MDRResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot


Library     ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library     ${LIBS_DIRECTORY}/WarehouseUtils.py
Library     DateTime
Suite Teardown  Cleanup System Ca Certs
Default Tags  CENTRAL  MCS

*** Test Cases ***

MCS Recieves Update Now Action From Central Correctly
    [Documentation]  Test is used to split update action from rest of tests to make them more stable
    Setup Fresh Install Nova
    Require Warehouse In Localhost
    Require Registered   waitForALCPolicy=${True}

    Remove File   ${SOPHOS_INSTALL}/logs/base/suldownloader.log

    Wait Until Keyword Succeeds
    ...  180 secs
    ...  60 secs
    ...  Update and check

MCS Reports Failure To Update From Central Credentials Correctly
    [Documentation]  Derived from CLOUD.UPD.004_Update_now.sh
    Setup Fresh Install Nova
    Require Warehouse In Localhost

    # Register with Nova and wait for ALC policy so that it doesn't come down and interfere with update tests
    Require Registered  waitForALCPolicy=${True}
    ${starttime} =  Get Cloud Time

    Wait Nova Report New UpdateSuccess   1
    Check Server Last Success Update Time  ${starttime}

    Stop Update Server
    Simulate Update Now
    Wait Nova Report New UpdateFailure  2


MCS Obtains ALC Policy From Central Credentials Correctly
    [Documentation]  Derived from CLOUD.POLICY.UPDATE.001_verify_config.sh
    Setup Fresh Install Nova
    # Will start up update server which is required for the test to function correctly.
    Require Warehouse In Localhost
    Require Registered   waitForALCPolicy=${True}
    Wait For Sul Config File To Be Updated From Policy
    @{policyValues} =  Get Alc Policy File Info  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    Log File   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    @{configValues} =  Get Sul Config File Info  ${sulConfigPath}
    Lists Should Be Equal  ${policyValues}  ${configValues}


*** Keywords ***
Update and check
    Request Nova To Send Update Now Action
    Wait For Update Action To Be Processed

Clean Mcs Envelope Log File
    ${result} =  Run Process   >   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log   shell=True
    Should Be Equal As Integers  ${result.rc}   0   msg=Failed to truncate mcs_envelope log file