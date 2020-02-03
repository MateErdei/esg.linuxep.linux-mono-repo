*** Settings ***
Documentation    Check MCS handles updates via Nova Central

Resource  ../mcs_router/McsRouterResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  McsRouterNovaResources.robot
Resource  ../mdr_plugin/MDRResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot


Library     ${libs_directory}/WarehouseGenerator.py
Library     ${libs_directory}/FullInstallerUtils.py
Library     ${libs_directory}/CentralUtils.py
Library     ${libs_directory}/LogUtils.py
Library     ${libs_directory}/UpdateSchedulerHelper.py
Library     ${libs_directory}/WarehouseUtils.py
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


MCS Reports Update From Central Credentials Correctly
    [Documentation]  Derived from CLOUD.UPD.001_Update_now.sh
    Setup Fresh Install Nova
    Require Warehouse In Localhost
    Require Registered    waitForALCPolicy=${True}
    Simulate Update Now
    Wait Nova Report New UpdateSuccess   1

Nova Reports Reboot Required When MCS Sends Reboot Required Event
    [Documentation]  Derived from CLOUD.UPD.002_Reboot_required.sh
    Setup Fresh Install Nova
    Require Registered    waitForALCPolicy=${True}
    ${rebootevent}=  Set Variable  ${SUPPORT_FILES}/CentralXml/ALC_reboot_required_event.xml
    Send Event File  ALC  ${rebootevent}
    Wait Nova Report New UpdateReboot

MCS Reports Update From Central Credentials Correctly and Includes Update Time
    [Documentation]  Derived from CLOUD.UPD.003_success_report.sh extends : CLOUD.UPD.001_Update_now.sh
    Setup Fresh Install Nova
    Require Warehouse In Localhost
    Require Registered   waitForALCPolicy=${True}
    Simulate Update Now
    ${starttime} =  Get Cloud Time
    Wait Nova Report New UpdateSuccess   1
    Check Server Last Success Update Time  ${starttime}

MCS Reports Failure To Update From Central Credentials Correctly
    [Documentation]  Derived from CLOUD.UPD.004_Update_now.sh
    Setup Fresh Install Nova
    Require Warehouse In Localhost

    # Register with Nova and wait for ALC policy so that it doesn't come down and interfere with update tests
    Require Registered  waitForALCPolicy=${True}
    Simulate Update Now
    ${starttime} =  Get Cloud Time

    Wait Nova Report New UpdateSuccess   1
    Check Server Last Success Update Time  ${starttime}

    Stop Update Server
    Simulate Update Now
    Wait Nova Report New UpdateFailure  2


Update Via Local Update Cache
    [Documentation]  Derived from CLOUD.UPD.013_Update_via_update_cache.sh *and* CLOUD.UPD.012_Update_cache_settings.sh
    [Tags]  CENTRAL   MCS  EXCLUDE_AWS
    Setup Fresh Install Nova
    Require Update Cache Warehouse In Localhost
    Require Registered   waitForALCPolicy=${True}
    Wait Until Keyword Succeeds
    ...  140
    ...  35
    ...  Wait for connection to update cache


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

Multiple Updates With Reregisters Should Be Reported To Central
    [Documentation]  Derived from CLOUD.UPD.016_report_last_update_to_central.sh
    ## This test is prone to failing due to nova flakeyness therefore
    ## we will run this manually at the end of each test sprint
    [Tags]  MDR_PLUGIN  CENTRAL  MANUAL
    Setup Fresh Install Nova
    Require Warehouse In Localhost
    Require Registered    waitForALCPolicy=${True}

    Simulate Update Now
    Wait Nova Report New UpdateSuccess   1

    Deregister From Central
    Remove File  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    Require Registered

    Simulate Update Now
    Wait Nova Report New UpdateSuccess   2

    Deregister From Central
    Remove File  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    Require Registered

    Simulate Update Now
    Wait Nova Report New UpdateSuccess   3

    Deregister From Central
    Remove File  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    Require Registered

    Simulate Update Now
    Wait Nova Report New UpdateSuccess   4

MDR Plugin Installed On Update And Status Sent On Receiving MDR Policy From Nova
    [Documentation]  Derived from CLOUD.UPD.016_report_last_update_to_central.sh
    Setup Fresh Install Nova
    Require Warehouse In Localhost
    Require Registered    waitForALCPolicy=${True}

    Simulate Update Now

    Wait Nova Report New UpdateSuccess   1

    Wait until MDR policy is Downloaded
    Wait For MDR Status

    ${MDRStatus} =  Get File  ${MDR_STATUS_XML}
    Should Contain  ${MDRStatus}   Res='Same'
    Should Not Contain  ${MDRStatus}   RevID=''


MDR Plugin Installed and then uninstalled when registered to a non-MDR account
    [Documentation]  Install MDR when using an MDR enabled account, then switch accounts to a non-MDR account (dev@savlinux.test.com) which will uninstall MDR.
    #this test case is not supported
    [Tags]  MDR_PLUGIN  CENTRAL  MANUAL
    Setup Fresh Install Nova
    Require Warehouse In Localhost
    Require Registered

    Simulate Update Now
    Wait Nova Report New UpdateSuccess   1
    Wait For MDR To Be Installed
    Wait until MDR policy is Downloaded
    Wait For MDR Status
    Register With Non MDR Account
    Wait For MCS Router To Be Running
    Wait For Server In Cloud
    Wait For ALC Policy To Not Contain MDR

    Simulate Update Now
    Wait For Update Action To Be Processed
    Wait for SulDownloader Report
    Wait Until MDR Uninstalled


MDR Plugin not installed and then installed when registered to an MDR account
    [Documentation]   Do not install MDR when using a non-MDR enabled account, then switch accounts to an MDR account which will then install MDR.
    #this test case is not supported
    [Tags]  MDR_PLUGIN  CENTRAL  MANUAL
    # Non-MDR install
    Require Fresh Install
    Require Warehouse In Localhost
    Register With Non MDR Account
    Check MDR Plugin Uninstalled
    Wait For Server In Cloud

    # Reregister to MDR account
    Wait For ALC Policy To Not Contain MDR
    Register With MDR Account
    Wait For ALC Policy To Contain MDR

    # Update
    Simulate Update Now
    Wait For MCS Router To Be Running

    # Expect MDR installed and running
    Wait For MDR To Be Installed
    Wait until MDR policy is Downloaded
    Wait For MDR Status

*** Keywords ***
Update and check
    Request Nova To Send Update Now Action
    Wait For Update Action To Be Processed

Clean Mcs Envelope Log File
    ${result} =  Run Process   >   ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log   shell=True
    Should Be Equal As Integers  ${result.rc}   0   msg=Failed to truncate mcs_envelope log file