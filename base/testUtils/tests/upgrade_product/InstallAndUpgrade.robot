*** Settings ***
Suite Setup      Suite Setup
Suite Teardown   Suite Teardown

Test Setup       Test Setup
Test Teardown    Test Teardown

Test Timeout  10 mins

Library     ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library     ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${LIBS_DIRECTORY}/WarehouseUtils.py
Library     ${LIBS_DIRECTORY}/TeardownTools.py
Library     ${LIBS_DIRECTORY}/UpgradeUtils.py
Library     Process
Library     OperatingSystem
Library     String

Resource    ../event_journaler/EventJournalerResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../thin_installer/ThinInstallerResources.robot
Resource    ../example_plugin/ExamplePluginResources.robot
Resource    ../av_plugin/AVResources.robot
Resource    ../mdr_plugin/MDRResources.robot
Resource    ../edr_plugin/EDRResources.robot
Resource    ../runtimedetections_plugin/RuntimeDetectionsResources.robot
Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    UpgradeResources.robot
Resource    ../management_agent/ManagementAgentResources.robot

*** Variables ***
${BaseEdrAndMtrAndAVDogfoodPolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT-1.xml
${BaseEdrAndMtrAndAVReleasePolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_Release.xml
${BaseAndMtrVUTPolicy}                      ${GeneratedWarehousePolicies}/base_and_mtr_VUT.xml
${BaseEdrAndMtrAndAVVUTPolicy}              ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT.xml
${BaseOnlyVUT_weekly_schedule_Policy}       ${GeneratedWarehousePolicies}/base_only_weeklyScheduleVUT.xml

${LocalWarehouseDir}                        ./local_warehouses
${SULDownloaderLog}                         ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${SULDownloaderLogDowngrade}                ${SOPHOS_INSTALL}/logs/base/downgrade-backup/suldownloader.log
${UpdateSchedulerLog}                       ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
${Sophos_Scheduled_Query_Pack}              ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
${status_file}                              ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

*** Test Cases ***

We Can Install From A Ballista Warehouse
    [Tags]  MANUAL

    # SETTINGS
    # ---------------------------------------------------------------------------------------
    #  warehouse creds
    #  SET THESE
    ${username} =  Set Variable  username
    ${password} =  Set Variable  password

    # ALC Policy Types
    ${Base} =  Set Variable         ${GeneratedWarehousePolicies}/base_only_VUT.xml
    ${BaseMtr} =  Set Variable      ${GeneratedWarehousePolicies}/base_and_mtr_VUT.xml
    ${BaseEdr} =  Set Variable      ${GeneratedWarehousePolicies}/base_and_edr_VUT.xml
    ${BaseEdrMTr} =  Set Variable   ${GeneratedWarehousePolicies}/base_edr_and_mtr.xml
    # CHANGE THIS ONE
    ${policy_template} =  Set Variable  ${BaseEdrMTr}
    # Make sure the policy template you picked is appropriate for the warehouse you want to test.
    # If you can't find one, make one.
    # ---------------------------------------------------------------------------------------
    # SETTINGS

    ${ballista_policy} =  Create Ballista Config From Template Policy  ${policy_template}  ${username}  ${password}
    Log File  ${ballista_policy}

    Start Local Cloud Server  --initial-alc-policy  ${ballista_policy}

    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${ballista_policy}

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Check Suldownloader Log Contains  Successfully connected to: Sophos at https://dci.sophosupd.com/cloudupdate

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

#    Force A failure If you want to check for anything with the teardown logs
#    Fail

We Can Upgrade From Dogfood to VUT Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVDogfoodPolicy}

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseEdrAndMtrAndAVDogfoodPolicy}  force_legacy_install=${True}
    Override Local LogConf File Using Content  [suldownloader]\nVERBOSITY = DEBUG\n

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Run Shell Process   /opt/sophos-spl/bin/wdctl stop av     OnError=Failed to stop av
    Override LogConf File as Global Level  DEBUG
    Run Shell Process   /opt/sophos-spl/bin/wdctl start av    OnError=Failed to start av

    Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log  suldownloader_log   Update success  1

    Check EAP Release With AV Installed Correctly
    ${ExpectedBaseDevVersion} =     get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Base-component
    ${ExpectedBaseReleaseVersion} =     get_version_from_warehouse_for_rigidname_in_componentsuite  ${BaseEdrAndMtrAndAVDogfoodPolicy}  ServerProtectionLinux-Base-component  ServerProtectionLinux-Base
    ${BaseReleaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    Should Be Equal As Strings  ${ExpectedBaseReleaseVersion}  ${BaseReleaseVersion}
    ${ExpectedMtrDevVersion} =      get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-MDR
    ${ExpectedMtrReleaseVersion} =      get_version_from_warehouse_for_rigidname  ${BaseEdrAndMtrAndAVDogfoodPolicy}  ServerProtectionLinux-Plugin-MDR
    ${MtrReleaseVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedMtrReleaseVersion}  ${MtrReleaseVersion}
    ${ExpectedEdrDevVersion} =      get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-EDR
    ${ExpectedEdrReleaseVersion} =      get_version_from_warehouse_for_rigidname  ${BaseEdrAndMtrAndAVDogfoodPolicy}  ServerProtectionLinux-Plugin-EDR
    ${EdrReleaseVersion} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedEdrReleaseVersion}  ${EdrReleaseVersion}
    ${ExpectedLRDevVersion} =      get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-liveresponse
    ${ExpectedLRReleaseVersion} =      get_version_from_warehouse_for_rigidname_in_componentsuite  ${BaseEdrAndMtrAndAVDogfoodPolicy}  ServerProtectionLinux-Plugin-liveresponse  ServerProtectionLinux-Base
    ${LrReleaseVersion} =      Get Version Number From Ini File   ${InstalledLRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedLRReleaseVersion}  ${LRReleaseVersion}
    ${ExpectedAVDevVersion} =       get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-AV
    ${ExpectedAVReleaseVersion} =      get_version_from_warehouse_for_rigidname  ${BaseEdrAndMtrAndAVDogfoodPolicy}  ServerProtectionLinux-Plugin-AV
    ${AVReleaseVersion} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedAVReleaseVersion}  ${AVReleaseVersion}
    ${ExpectedRuntimedetectionsDevVersion} =       get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-RuntimeDetections
    ${ExpectedRuntimedetectionsReleaseVersion} =      get_version_from_warehouse_for_rigidname_in_componentsuite  ${BaseEdrAndMtrAndAVDogfoodPolicy}  ServerProtectionLinux-Plugin-RuntimeDetections  ServerProtectionLinux-Base
    ${RuntimeDetectionsReleaseVersion} =      Get Version Number From Ini File   ${InstalledRuntimedetectionsPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedRuntimedetectionsReleaseVersion}  ${RuntimeDetectionsReleaseVersion}
    ${ExpectedEJDevVersion} =    get_version_for_rigidname_in_vut_warehouse    ServerProtectionLinux-Plugin-EventJournaler
    ${ExpectedEJReleaseVersion} =    get_version_from_warehouse_for_rigidname_in_componentsuite    ${BaseEdrAndMtrAndAVDogfoodPolicy}    ServerProtectionLinux-Plugin-EventJournaler    ServerProtectionLinux-Base
    ${EJReleaseVersion} =      Get Version Number From Ini File    ${InstalledEJPluginVersionFile}
    Should Be Equal As Strings    ${ExpectedEJReleaseVersion}    ${EJReleaseVersion}

    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVVUTPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseEdrAndMtrAndAVVUTPolicy}
    Wait until threat detector running

    ${HealthyShsStatusXmlContents} =  Set Variable  <item name="health" value="1" />

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  15 secs
    ...  SHS Status File Contains  ${HealthyShsStatusXmlContents}

    Mark Watchdog Log
    Mark Managementagent Log
    Start Process  tail -f ${SOPHOS_INSTALL}/logs/base/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true
    Trigger Update Now


    SHS Status File Contains  ${HealthyShsStatusXmlContents}

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    /tmp/preserve-sul-downgrade   suldownloader_log   Update success  2

    SHS Status File Contains  ${HealthyShsStatusXmlContents}

    #confirm that the warehouse flags supplement is installed when upgrading
    File Exists With Permissions  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  root  sophos-spl-group  -rw-r-----

    Check Watchdog Service File Has Correct Kill Mode

    Mark Known Upgrade Errors

    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found

    #TODO LINUXDAR-2972 remove when this defect is fixed
    #not an error should be a WARN instead, but it's happening on the EAP version so it's too late to change it now
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value

    #this is expected because we are restarting the avplugin to enable debug logs, we need to make sure it occurs only once though
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> Exiting sophos_threat_detector with code: 15

    #TODO LINUXDAR-5140 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> failure in ConfigMonitor: pselect failed: Bad file descriptor
    Run Keyword And Expect Error  *
    ...     Check Log Contains String N  times ${SOPHOS_INSTALL}/plugins/av/log/av.log  av.log  Exiting sophos_threat_detector with code: 15  2

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Check Current Release With AV Installed Correctly
    Wait For RuntimeDetections to be Installed

    ${BaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    Should Be Equal As Strings  ${ExpectedBaseDevVersion}  ${BaseDevVersion}
    ${MtrDevVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedMtrDevVersion}  ${MtrDevVersion}
    ${EDRDevVersion} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedEDRDevVersion}  ${EDRDevVersion}
    ${LRDevVersion} =      Get Version Number From Ini File   ${InstalledLRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedLRDevVersion}  ${LRDevVersion}
    ${AVDevVersion} =       Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedAVDevVersion}  ${AVDevVersion}
    ${RuntimedetectionsVersion} =      Get Version Number From Ini File   ${InstalledRuntimedetectionsPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedRuntimedetectionsDevVersion}  ${RuntimedetectionsVersion}
    ${EJDevVersion} =       Get Version Number From Ini File    ${InstalledEJPluginVersionFile}
    Should Be Equal As Strings    ${ExpectedEJDevVersion}    ${EJDevVersion}

    Check Event Journaler Executable Running
    Check AV Plugin Permissions
    Check Update Reports Have Been Processed
    SHS Status File Contains  ${HealthyShsStatusXmlContents}

    # This will turn health bad because "Check AV Plugin Can Scan Files" scans an eicar.
    Check AV Plugin Can Scan Files
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  3 secs
    ...  SHS Status File Contains  <item name="threat" value="2" />

We Can Downgrade From VUT to Dogfood Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]   INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA   BASE_DOWNGRADE

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseEdrAndMtrAndAVVUTPolicy}

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    Start Process  tail -fn0 ${SOPHOS_INSTALL}/logs/base/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true
    Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  1

    Check Current Release Installed Correctly
    ${ExpectedBaseDevVersion} =     get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Base-component
    ${ExpectedBaseReleaseVersion} =     get_version_from_warehouse_for_rigidname_in_componentsuite  ${BaseEdrAndMtrAndAVDogfoodPolicy}  ServerProtectionLinux-Base-component  ServerProtectionLinux-Base
    ${BaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${ExpectBaseDowngrade} =  second_version_is_lower  ${ExpectedBaseDevVersion}  ${ExpectedBaseReleaseVersion}
    Should Be Equal As Strings  ${ExpectedBaseDevVersion}  ${BaseVersion}
    ${ExpectedMtrDevVersion} =      get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-MDR
    ${ExpectedMtrReleaseVersion} =      get_version_from_warehouse_for_rigidname  ${BaseEdrAndMtrAndAVDogfoodPolicy}  ServerProtectionLinux-Plugin-MDR
    ${MtrVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedMtrDevVersion}  ${MtrVersion}
    ${ExpectedAVDevVersion} =       get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-AV
    ${ExpectedAVReleaseVersion} =      get_version_from_warehouse_for_rigidname  ${BaseEdrAndMtrAndAVDogfoodPolicy}  ServerProtectionLinux-Plugin-AV
    ${AVVersion} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedAVDevVersion}  ${AVVersion}
    ${ExpectedEDRDevVersion} =       get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-EDR
    ${ExpectedEDRReleaseVersion} =      get_version_from_warehouse_for_rigidname  ${BaseEdrAndMtrAndAVDogfoodPolicy}  ServerProtectionLinux-Plugin-EDR
    ${EdrVersion} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedEDRDevVersion}  ${EdrVersion}
    ${ExpectedLRDevVersion} =      get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-liveresponse
    ${ExpectedLRReleaseVersion} =      get_version_from_warehouse_for_rigidname_in_componentsuite  ${BaseEdrAndMtrAndAVDogfoodPolicy}  ServerProtectionLinux-Plugin-liveresponse  ServerProtectionLinux-Base
    ${LrDevVersion} =      Get Version Number From Ini File   ${InstalledLRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedLRDevVersion}  ${LrDevVersion}
    ${ExpectedEJDevVersion} =    get_version_for_rigidname_in_vut_warehouse    ServerProtectionLinux-Plugin-EventJournaler
    ${ExpectedEJReleaseVersion} =    get_version_from_warehouse_for_rigidname_in_componentsuite    ${BaseEdrAndMtrAndAVDogfoodPolicy}    ServerProtectionLinux-Plugin-EventJournaler    ServerProtectionLinux-Base
    ${EJDevVersion} =      Get Version Number From Ini File    ${InstalledEJPluginVersionFile}
    Should Be Equal As Strings    ${ExpectedEJDevVersion}    ${EJDevVersion}
    ${ExpectedRuntimedetectionsDevVersion} =       get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-RuntimeDetections
    ${ExpectedRuntimedetectionsReleaseVersion} =      get_version_from_warehouse_for_rigidname_in_componentsuite  ${BaseEdrAndMtrAndAVDogfoodPolicy}  ServerProtectionLinux-Plugin-RuntimeDetections  ServerProtectionLinux-Base
    ${RuntimeDetectionsDevVersion} =      Get Version Number From Ini File   ${InstalledRuntimedetectionsPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedRuntimedetectionsDevVersion}  ${RuntimeDetectionsDevVersion}

    Directory Should Not Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup

    # Products that should be uninstalled after downgrade
    Should Exist  ${InstalledLRPluginVersionFile}

    #the query pack should have been installed with EDR VUT
    Should Exist  ${Sophos_Scheduled_Query_Pack}

    ${sspl_user_uid} =  Get UID From Username  sophos-spl-user
    ${sspl_local_uid} =  Get UID From Username  sophos-spl-local
    ${sspl_network_uid} =  Get UID From Username  sophos-spl-network
    ${sspl_update_uid} =  Get UID From Username  sophos-spl-updatescheduler

    # Changing the policy here will result in an automatic update
    # Note when downgrading from a release with live response to a release without live response
    # results in a second update.
    Override LogConf File as Global Level  DEBUG

    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVDogfoodPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseEdrAndMtrAndAVDogfoodPolicy}

    Mark Watchdog Log
    Mark Managementagent Log
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains   Update success    /tmp/preserve-sul-downgrade   suldownloader log

    Run Keyword If  ${ExpectBaseDowngrade}
    ...  Check Log Contains  Preparing ServerProtectionLinux-Base-component for downgrade  ${SULDownloaderLogDowngrade}  backedup suldownloader log

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  10 secs
    ...  Check Log Contains String At Least N Times   /tmp/preserve-sul-downgrade  Downgrade Log  Update success  1
    #Wait for successful update (all up to date) after downgrading
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  10 secs
    ...  Check Log Contains String At Least N Times   ${SULDownloaderLog}  Update Log  Update success  1

    Check for Management Agent Failing To Send Message To MTR And Check Recovery

    Mark Known Upgrade Errors

    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log  updatescheduler <> Update Service (sophos-spl-update.service) failed

    #TODO LINUXDAR-2972 remove when this defect is fixed
    #not an error should be a WARN instead, but it's happening on the EAP version so it's too late to change it now
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    #TODO LINUXDAR-5140 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> failure in ConfigMonitor: pselect failed: Bad file descriptor


    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Log File  /tmp/preserve-sul-downgrade
    Check EAP Release With AV Installed Correctly

    ${BaseReleaseVersion} =                     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrReleaseVersion} =                      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    ${EdrReleaseVersion} =                      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    ${AVReleaseVersion} =                       Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    ${LRReleaseVersion} =                       Get Version Number From Ini File   ${InstalledLRPluginVersionFile}
    ${RuntimedetectionsReleaseVersion} =        Get Version Number From Ini File   ${InstalledRuntimedetectionsPluginVersionFile}
    ${EJReleaseVersion} =                       Get Version Number From Ini File   ${InstalledEJPluginVersionFile}

    Should Be Equal As Strings      ${BaseReleaseVersion}               ${ExpectedBaseReleaseVersion}
    Should Be Equal As Strings      ${MtrReleaseVersion}                ${ExpectedMtrReleaseVersion}
    Should Be Equal As Strings      ${EdrReleaseVersion}                ${ExpectedEDRReleaseVersion}
    Should Be Equal As Strings      ${AVReleaseVersion}                 ${ExpectedAVReleaseVersion}
    Should Be Equal As Strings      ${LRReleaseVersion}                 ${ExpectedLRReleaseVersion}
    Should Be Equal As Strings      ${RuntimedetectionsReleaseVersion}  ${ExpectedRuntimedetectionsReleaseVersion}
    Should Be Equal As Strings      ${EJReleaseVersion}                 ${ExpectedEJReleaseVersion}


    #Check users haven't been removed and added back
    ${new_sspl_user_uid} =  Get UID From Username  sophos-spl-user
    ${new_sspl_local_uid} =  Get UID From Username  sophos-spl-local
    ${new_sspl_network_uid} =  Get UID From Username  sophos-spl-network
    ${new_sspl_update_uid} =  Get UID From Username  sophos-spl-updatescheduler
    Should Be Equal As Integers  ${sspl_user_uid}  ${new_sspl_user_uid}
    Should Be Equal As Integers  ${sspl_local_uid}  ${new_sspl_local_uid}
    Should Be Equal As Integers  ${sspl_network_uid}  ${new_sspl_network_uid}
    Should Be Equal As Integers  ${sspl_update_uid}  ${new_sspl_update_uid}

    Start Process  tail -fn0 ${SOPHOS_INSTALL}/logs/base/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true
    # Upgrade back to master to check we can upgrade from a downgraded product
    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  150 secs
    ...  10 secs
    ...  version_number_in_ini_file_should_be  ${InstalledBaseVersionFile}    ${ExpectedBaseDevVersion}

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be  ${InstalledMDRPluginVersionFile}    ${ExpectedMtrDevVersion}

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be  ${InstalledEDRPluginVersionFile}    ${ExpectedEDRDevVersion}

    #wait for AV plugin to be running before attempting uninstall
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be  ${InstalledAVPluginVersionFile}    ${ExpectedAVDevVersion}

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be  ${InstalledRuntimedetectionsPluginVersionFile}    ${ExpectedRuntimedetectionsDevVersion}

    ${BaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    Should Be Equal As Strings  ${ExpectedBaseDevVersion}  ${BaseVersion}
    ${MtrVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedMtrDevVersion}  ${MtrVersion}
    ${AVVersion} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedAVDevVersion}  ${AVVersion}
    ${EdrVersion} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedEDRDevVersion}  ${EdrVersion}
    ${RuntimeDetectionsVersion} =      Get Version Number From Ini File   ${InstalledRuntimedetectionsPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedRuntimedetectionsDevVersion}  ${RuntimeDetectionsVersion}
    ${EJVersion} =    Get Version Number From Ini File    ${InstalledEJPluginVersionFile}
    Should Be Equal As Strings    ${ExpectedEJDevVersion}    ${EJVersion}

    #the query pack should have been re-installed
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  file should exist  ${Sophos_Scheduled_Query_Pack}

We Can Upgrade From Release to VUT Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVReleasePolicy}

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseEdrAndMtrAndAVReleasePolicy}  force_legacy_install=${True}
    Override Local LogConf File Using Content  [suldownloader]\nVERBOSITY = DEBUG\n

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Run Shell Process   /opt/sophos-spl/bin/wdctl stop av     OnError=Failed to stop av
    Override LogConf File as Global Level  DEBUG
    Run Shell Process   /opt/sophos-spl/bin/wdctl start av    OnError=Failed to start av

    Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log  suldownloader_log   Update success  1

    Check EAP Release With AV Installed Correctly
    ${ExpectedBaseDevVersion} =     get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Base-component
    ${ExpectedBaseReleaseVersion} =     get_version_from_warehouse_for_rigidname_in_componentsuite  ${BaseEdrAndMtrAndAVReleasePolicy}  ServerProtectionLinux-Base-component  ServerProtectionLinux-Base
    ${BaseReleaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    Should Be Equal As Strings  ${ExpectedBaseReleaseVersion}  ${BaseReleaseVersion}
    ${ExpectedMtrDevVersion} =      get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-MDR
    ${ExpectedMtrReleaseVersion} =      get_version_from_warehouse_for_rigidname  ${BaseEdrAndMtrAndAVReleasePolicy}  ServerProtectionLinux-Plugin-MDR
    ${MtrReleaseVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedMtrReleaseVersion}  ${MtrReleaseVersion}
    ${ExpectedAVDevVersion} =       get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-AV
    ${ExpectedAVReleaseVersion} =      get_version_from_warehouse_for_rigidname  ${BaseEdrAndMtrAndAVReleasePolicy}  ServerProtectionLinux-Plugin-AV
    ${AVReleaseVersion} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedAVReleaseVersion}  ${AVReleaseVersion}
    ${ExpectedEdrDevVersion} =      get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-EDR
    ${ExpectedEdrReleaseVersion} =      get_version_from_warehouse_for_rigidname  ${BaseEdrAndMtrAndAVReleasePolicy}  ServerProtectionLinux-Plugin-EDR
    ${EdrReleaseVersion} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedEdrReleaseVersion}  ${EdrReleaseVersion}
    ${ExpectedLRDevVersion} =      get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-liveresponse
    ${ExpectedLRReleaseVersion} =      get_version_from_warehouse_for_rigidname_in_componentsuite  ${BaseEdrAndMtrAndAVReleasePolicy}  ServerProtectionLinux-Plugin-liveresponse  ServerProtectionLinux-Base
    ${LrReleaseVersion} =      Get Version Number From Ini File   ${InstalledLRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedLRReleaseVersion}  ${LRReleaseVersion}
    ${ExpectedRuntimedetectionsDevVersion} =      get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-RuntimeDetections
    ${ExpectedEJDevVersion} =    get_version_for_rigidname_in_vut_warehouse    ServerProtectionLinux-Plugin-EventJournaler

    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVVUTPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseEdrAndMtrAndAVVUTPolicy}
    Wait until threat detector running

# TODO LINUXDAR-4263: Uncomment this when SSPL Shipping has Health
#    ${HealthyShsStatusXmlContents} =  Set Variable  <item name="health" value="1" />
#
#    Wait Until Keyword Succeeds
#    ...  60 secs
#    ...  15 secs
#    ...  SHS Status File Contains  ${HealthyShsStatusXmlContents}
#
    Mark Watchdog Log
    Mark Managementagent Log
    Start Process  tail -f ${SOPHOS_INSTALL}/logs/base/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true
    Trigger Update Now

# TODO LINUXDAR-4263: Uncomment this when SSPL Shipping has Health
#    Wait Until Keyword Succeeds
#    ...   60 secs
#    ...   10 secs
#    ...   Check Log Contains  Installing product: ServerProtectionLinux-Base-component  /tmp/preserve-sul-downgrade   suldownloader_log
#    SHS Status File Contains  ${HealthyShsStatusXmlContents}

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    /tmp/preserve-sul-downgrade   suldownloader_log   Update success  2

# TODO LINUXDAR-4263: Uncomment this when SSPL Shipping has Health
#    SHS Status File Contains  ${HealthyShsStatusXmlContents}

    #confirm that the warehouse flags supplement is installed when upgrading
    File Exists With Permissions  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  root  sophos-spl-group  -rw-r-----

    Check Mtr Reconnects To Management Agent After Upgrade
    Check for Management Agent Failing To Send Message To MTR And Check Recovery

    Mark Known Upgrade Errors

    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found
    #TODO LINUXDAR-2972 remove when this defect is fixed
    #not an error should be a WARN instead, but it's happening on the EAP version so it's too late to change it now
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value

    #this is expected because we are restarting the avplugin to enable debug logs, we need to make sure it occurs only once though
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> Exiting sophos_threat_detector with code: 15
    #TODO LINUXDAR-5140 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> failure in ConfigMonitor: pselect failed: Bad file descriptor
    Run Keyword And Expect Error  *
    ...     Check Log Contains String N  times ${SOPHOS_INSTALL}/plugins/av/log/av.log  av.log  Exiting sophos_threat_detector with code: 15  2

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Check Current Release With AV Installed Correctly

    ${BaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    Should Be Equal As Strings  ${ExpectedBaseDevVersion}  ${BaseDevVersion}
    ${MtrDevVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedMtrDevVersion}  ${MtrDevVersion}
    ${AVDevVersion} =       Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedAVDevVersion}  ${AVDevVersion}
    ${EDRDevVersion} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedEDRDevVersion}  ${EDRDevVersion}
    ${LRDevVersion} =      Get Version Number From Ini File   ${InstalledLRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedLRDevVersion}  ${LRDevVersion}
    ${RuntimedetectionsVersion} =      Get Version Number From Ini File   ${InstalledRuntimedetectionsPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedRuntimedetectionsDevVersion}  ${RuntimedetectionsVersion}
    ${EJDevVersion} =       Get Version Number From Ini File    ${InstalledEJPluginVersionFile}
    Should Be Equal As Strings    ${ExpectedEJDevVersion}    ${EJDevVersion}

    Check Event Journaler Executable Running
    Wait For RuntimeDetections to be Installed
    Check AV Plugin Permissions
    Check AV Plugin Can Scan Files
    Check Update Reports Have Been Processed

# TODO LINUXDAR-4263: Uncomment this when SSPL Shipping has Health
#    SHS Status File Contains  ${HealthyShsStatusXmlContents}


We Can Downgrade From VUT to Release Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]   INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA   BASE_DOWNGRADE

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseEdrAndMtrAndAVVUTPolicy}

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    Start Process  tail -fn0 ${SOPHOS_INSTALL}/logs/base/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true
    Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  1

    Check Current Release Installed Correctly
    ${ExpectedBaseDevVersion} =     get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Base-component
    ${ExpectedBaseReleaseVersion} =     get_version_from_warehouse_for_rigidname_in_componentsuite  ${BaseEdrAndMtrAndAVReleasePolicy}  ServerProtectionLinux-Base-component  ServerProtectionLinux-Base
    ${BaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${ExpectBaseDowngrade} =  second_version_is_lower  ${ExpectedBaseDevVersion}  ${ExpectedBaseReleaseVersion}
    Should Be Equal As Strings  ${ExpectedBaseDevVersion}  ${BaseVersion}
    ${ExpectedMtrDevVersion} =      get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-MDR
    ${ExpectedMtrReleaseVersion} =      get_version_from_warehouse_for_rigidname  ${BaseEdrAndMtrAndAVReleasePolicy}  ServerProtectionLinux-Plugin-MDR
    ${MtrVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedMtrDevVersion}  ${MtrVersion}
    ${ExpectedAVDevVersion} =       get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-AV
    ${ExpectedAVReleaseVersion} =      get_version_from_warehouse_for_rigidname  ${BaseEdrAndMtrAndAVReleasePolicy}  ServerProtectionLinux-Plugin-AV
    ${AVVersion} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedAVDevVersion}  ${AVVersion}
    ${ExpectedEDRDevVersion} =       get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-EDR
    ${ExpectedEDRReleaseVersion} =      get_version_from_warehouse_for_rigidname  ${BaseEdrAndMtrAndAVReleasePolicy}  ServerProtectionLinux-Plugin-EDR
    ${EdrVersion} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedEDRDevVersion}  ${EdrVersion}
    ${ExpectedLRDevVersion} =      get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-liveresponse
    ${ExpectedLRReleaseVersion} =      get_version_from_warehouse_for_rigidname_in_componentsuite  ${BaseEdrAndMtrAndAVReleasePolicy}  ServerProtectionLinux-Plugin-liveresponse  ServerProtectionLinux-Base
    ${LrDevVersion} =      Get Version Number From Ini File   ${InstalledLRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedLRDevVersion}  ${LrDevVersion}
    ${ExpectedRuntimedetectionsDevVersion} =      get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Plugin-RuntimeDetections
    ${RuntimeDetectionsDevVersion} =      Get Version Number From Ini File   ${InstalledRuntimedetectionsPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedRuntimedetectionsDevVersion}  ${RuntimeDetectionsDevVersion}
    ${ExpectedEJDevVersion} =    get_version_for_rigidname_in_vut_warehouse    ServerProtectionLinux-Plugin-EventJournaler
    ${EJDevVersion} =      Get Version Number From Ini File    ${InstalledEJPluginVersionFile}
    Should Be Equal As Strings    ${ExpectedEJDevVersion}    ${EJDevVersion}

    Directory Should Not Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup

    # Products that should be uninstalled after downgrade
    Should Exist  ${InstalledLRPluginVersionFile}

    #the query pack should have been installed with EDR VUT
    Should Exist  ${Sophos_Scheduled_Query_Pack}

    # Changing the policy here will result in an automatic update
    # Note when downgrading from a release with live response to a release without live response
    # results in a second update.
    Override LogConf File as Global Level  DEBUG
    Create File  ${SOPHOS_INSTALL}/base/mcs/action/testfile
    Should Exist  ${SOPHOS_INSTALL}/base/mcs/action/testfile
    Run Process  chown  -R  sophos-spl-local:sophos-spl-group  ${SOPHOS_INSTALL}/base/mcs/action/testfile

    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVReleasePolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseEdrAndMtrAndAVReleasePolicy}

    Mark Watchdog Log
    Mark Managementagent Log
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains   Update success    /tmp/preserve-sul-downgrade   suldownloader log

    Should Not Exist  ${SOPHOS_INSTALL}/base/mcs/action/testfile
    Run Keyword If  ${ExpectBaseDowngrade}
    ...  Check Log Contains  Preparing ServerProtectionLinux-Base-component for downgrade  ${SULDownloaderLogDowngrade}  backedup suldownloader log

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  10 secs
    ...  Check Log Contains String At Least N Times   /tmp/preserve-sul-downgrade  Downgrade Log  Update success  1
    #Wait for successful update (all up to date) after downgrading
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  10 secs
    ...  Check Log Contains String At Least N Times   ${SULDownloaderLog}  Update Log  Update success  1

    Check for Management Agent Failing To Send Message To MTR And Check Recovery

    Mark Known Upgrade Errors

    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log  updatescheduler <> Update Service (sophos-spl-update.service) failed
    #TODO LINUXDAR-2972 remove when this defect is fixed
    #not an error should be a WARN instead, but it's happening on the EAP version so it's too late to change it now
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    #TODO LINUXDAR-5140 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> failure in ConfigMonitor: pselect failed: Bad file descriptor

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Log File  /tmp/preserve-sul-downgrade
    Check EAP Release With AV Installed Correctly

    ${BaseReleaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrReleaseVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    ${EdrReleaseVersion} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    ${AVReleaseVersion} =       Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    ${LRReleaseVersion} =       Get Version Number From Ini File   ${InstalledLRPluginVersionFile}

    Should Be Equal As Strings      ${BaseReleaseVersion}  ${ExpectedBaseReleaseVersion}
    Should Be Equal As Strings      ${MtrReleaseVersion}   ${ExpectedMtrReleaseVersion}
    Should Be Equal As Strings      ${EdrReleaseVersion}   ${ExpectedEDRReleaseVersion}
    Should Be Equal As Strings      ${LRReleaseVersion}    ${ExpectedLRReleaseVersion}


    Start Process  tail -fn0 ${SOPHOS_INSTALL}/logs/base/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true
    # Upgrade back to master to check we can upgrade from a downgraded product
    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVVUTPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  150 secs
    ...  10 secs
    ...  version_number_in_ini_file_should_be  ${InstalledBaseVersionFile}    ${ExpectedBaseDevVersion}

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be  ${InstalledMDRPluginVersionFile}    ${ExpectedMtrDevVersion}

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be  ${InstalledEDRPluginVersionFile}    ${ExpectedEDRDevVersion}

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be  ${InstalledRuntimedetectionsPluginVersionFile}    ${ExpectedRuntimedetectionsDevVersion}

    #wait for AV plugin to be running before attempting uninstall
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be  ${InstalledAVPluginVersionFile}    ${ExpectedAVDevVersion}

    ${BaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    Should Be Equal As Strings  ${ExpectedBaseDevVersion}  ${BaseVersion}
    ${MtrVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedMtrDevVersion}  ${MtrVersion}
    ${AVVersion} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedAVDevVersion}  ${AVVersion}
    ${EdrVersion} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedEDRDevVersion}  ${EdrVersion}
    ${RuntimedetectionsVersion} =      Get Version Number From Ini File   ${InstalledRuntimedetectionsPluginVersionFile}
    Should Be Equal As Strings  ${ExpectedRuntimedetectionsDevVersion}  ${RuntimedetectionsVersion}
    ${EJVersion} =    Get Version Number From Ini File    ${InstalledEJPluginVersionFile}
    Should Be Equal As Strings    ${ExpectedEJDevVersion}    ${EJVersion}


    #the query pack should have been re-installed
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  file should exist  ${Sophos_Scheduled_Query_Pack}

Ensure Supplement Updates Only Perform A Supplement Update
    ## This can't run against real (remote) warehouses, since it modifies the warehouses to prevent product updates from working
    [Tags]  MANUAL
    [TearDown]  Cleanup Warehouse And Test Teardown
    [Timeout]    20 minutes

    ## This policy has a weekly schedule for 3am yesterday
    Start Local Cloud Server  --initial-alc-policy  ${BaseOnlyVUT_weekly_schedule_Policy}

    Log File  /etc/hosts
    Log File  ${BaseOnlyVUT_weekly_schedule_Policy}
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseOnlyVUT_weekly_schedule_Policy}

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    ## Running on a vagrant box - force local build in
#    Run Process  bash  +x  /vagrant/everest-base/output/SDDS-COMPONENT/install.sh
#    Run Process   rm  -rf  /opt/sophos-spl/base/VERSION*
#    Copy File  /opt/sophos-spl/base/update/cache/primary/ServerProtectionLinux-Base-component/VERSION.ini  /opt/sophos-spl/base/VERSION.ini

    ## Need to remove the log so that we can see if the later update is definitely supplement only in the logs
    Log File  ${SULDownloaderLog}
    Remove File  ${SULDownloaderLog}

    Disable Product Warehouse to ensure we only perform a supplement update  develop

    ## First update happens between 5-10 minutes after starting UpdateScheduler
    Sleep  5 minutes
    Wait Until Keyword Succeeds
    ...  5 minutes
    ...  30 secs
    ...  Check Log Contains In Order  ${SULDownloaderLog}
         ...  Doing supplement-only update
         ...  Update success


    Check Log Does Not Contain  Doing product and supplement update   ${SULDownloaderLog}  SulDownloaderLog
    Check Log Does Not Contain  Forcing product update due previous update failure or change in configuration   ${SULDownloaderLog}  SulDownloaderLog

Test That Only Subscriptions Appear As Subscriptions In ALC Status File
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA

    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrVUTPolicy}

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrVUTPolicy}

    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    Only Subscriptions In Policy Are In Alc Status Subscriptions


Check Installed Version In Status Message Is Correctly Reported Based On Version Ini File
    [Tags]  INSTALLER  THIN_INSTALLER  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA
    # Setup for test.
    Start Local Cloud Server  --initial-alc-policy  ${BaseAndMtrVUTPolicy}

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseAndMtrVUTPolicy}
    Wait Until Keyword Succeeds
    ...   60 secs
    ...   5 secs
    ...   File Should Exist  ${status_file}

    Override Local LogConf File for a component   INFO  global
    Run Process  systemctl  restart  sophos-spl

    # Tigger update to make sure everything has settle down foe test, i.e. we do not want files to be updated
    # when the actual test update is being performed.
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   240 secs
    ...   10 secs
    ...   Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success   2

    # Run test to simulate installedVersion content is correctly generated in status file.
    # First modify Base, live response and MTR version ini files
    # Base is set to an older version (make sure this is never newer than real installed version).
    #       Or installer will run.  This is to simulate new version failed to install.
    # Live reponse version.ini file will be removed, to simulate component never installed.
    # MTR is set to an older version (make sure this is never newer than real installed version).
    Remove File  ${SOPHOS_INSTALL}/base/VERSION.ini
    Create File   ${SOPHOS_INSTALL}/base/VERSION.ini   PRODUCT_VERSION = 1.0.123.123
    Run Process  chmod  640   ${SOPHOS_INSTALL}/base/VERSION.ini
    Run Process  chown  root:sophos-spl-group  ${SOPHOS_INSTALL}/base/VERSION.ini
    Remove File  ${SOPHOS_INSTALL}/plugins/liveresponse/VERSION.ini
    Remove File  ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini
    Create File  ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini   PRODUCT_VERSION = 1.0.0.234
    Remove File  ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/PRODUCT_VERSION.ini
    Create File  ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/PRODUCT_VERSION.ini  PRODUCT_VERSION = 1.0.0.345
    Remove File  ${SOPHOS_INSTALL}/plugins/mtr/osquery/VERSION.ini
    Create File  ${SOPHOS_INSTALL}/plugins/mtr/osquery/VERSION.ini  PRODUCT_VERSION = 1.0.0.456

    #  This update should not install any files.
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   240 secs
    ...   10 secs
    ...   Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success   3

    # Check content of ALC_status.xml file contsins the expected installedVersion values.
    Check Status File Component Installed Version Is Correct  ServerProtectionLinux-Base-component  1.0.123.123  ${status_file}
    Check Status File Component Installed Version Is Correct  ServerProtectionLinux-Plugin-liveresponse  ${EMPTY}  ${status_file}
    Check Status File Component Installed Version Is Correct  ServerProtectionLinux-Plugin-MDR  1.0.0.234  ${status_file}

*** Keywords ***

Cleanup Warehouse And Test Teardown
    Restore Product Warehouse  develop
    Test Teardown

Simulate Previous Scheduled Update Success
    # Trigger updating via Central Update Scheduler which will create the previous_update_config.json file only when
    # update scheduler process expected current reports rather than unprocessed old reports
    Run Process   cp  ${UPDATE_CONFIG}   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json
    Run Process   chown  sophos-spl-updatescheduler:sophos-spl-group   ${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json

Check for Management Agent Failing To Send Message To MTR And Check Recovery
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log  managementagent <> Failure on sending message to mtr. Reason: No incoming data
    ${EvaluationBool} =  Does Management Agent Log Contain    managementagent <> Failure on sending message to mtr. Reason: No incoming data
    Run Keyword If
    ...  ${EvaluationBool} == ${True}
    ...  Check For MTR Recovery

Check For MTR Recovery
    Remove File  ${MDR_PLUGIN_PATH}/var/policy/mtr.xml
    Check MDR Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  Check Marked Watchdog Log Contains   watchdog <> Starting mtr
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Marked Managementagent Log Contains   managementagent <> Policy ${SOPHOS_INSTALL}/base/mcs/policy/MDR_policy.xml applied to 1 plugins
    Check File Exists  ${MDR_PLUGIN_PATH}/var/policy/mtr.xml

Replace Versig Update Cert With Eng Certificate
    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${UPDATE_ROOTCERT_DIR}/ps_rootca.crt


Check Installed Version Files Against Expected
    [Arguments]  ${WarehousePath}
    ${WH_BASE} =  Get File  ${WarehousePath}/${WarehouseBaseVersionFileExtension}
    ${WH_MDR_PLUGIN} =  Get File  ${WarehousePath}/${WarehouseMDRPluginVersionFileExtension}
    ${WH_MDR_SUITE} =  Get File  ${WarehousePath}/${WarehouseMDRSuiteVersionFileExtension}

    ${INSTALLED_BASE} =  Get File  ${InstalledBaseVersionFile}
    ${INSTALLED_MDR_PLUGIN} =  Get File  ${InstalledMDRPluginVersionFile}
    ${INSTALLED_MDR_SUITE} =  Get File  ${InstalledMDRSuiteVersionFile}

    Should Be Equal As Strings  ${WH_BASE}  ${INSTALLED_BASE}
    Should Be Equal As Strings  ${WH_MDR_PLUGIN}  ${INSTALLED_MDR_PLUGIN}
    Should Be Equal As Strings  ${WH_MDR_SUITE}  ${INSTALLED_MDR_SUITE}

Redirect Sulconfig To Use Local Warehouse
    Replace Sophos URLS to Localhost  config_path=${UPDATE_CONFIG}
    Replace Username And Password In Sulconfig  config_path=${UPDATE_CONFIG}

Check Old MCS Router Running
    ${pid} =  Check MCS Router Process Running  require_running=True
    [return]  ${pid}

Check Current Release Installed Correctly
    Check Mcs Router Running
    Check MDR Plugin Installed
    Check Installed Correctly

Check EAP Release Installed Correctly
    Check MCS Router Running
    Check MDR Plugin Installed
    Check Installed Correctly


Check Current Release With AV Installed Correctly
    Check Mcs Router Running
    Check MDR Plugin Installed
    Check AV Plugin Installed
    Check Installed Correctly

Check EAP Release With AV Installed Correctly
    Check MCS Router Running
    Check MDR Plugin Installed
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  MDR Plugin Log Contains  Run SophosMTR
    Check AV Plugin Installed
    Check Installed Correctly


Check Installed Correctly
    Should Exist    ${SOPHOS_INSTALL}

    Check Correct MCS Password And ID For Local Cloud Saved

    ${result}=  Run Process  stat  -c  "%A"  /opt
    ${ExpectedPerms}=  Set Variable  "drwxr-xr-x"
    Should Be Equal As Strings  ${result.stdout}  ${ExpectedPerms}
    ${version_number} =  Get Version Number From Ini File  ${InstalledBaseVersionFile}
    Check Expected Base Processes Except SDU Are Running

Component Version has changed
    [Arguments]  ${oldVersion}  ${InstalledVersionFile}
    ${NewDevVersion} =  Get Version Number From Ini File   ${InstalledVersionFile}
    Should Not Be Equal As Strings  ${oldVersion}  ${NewDevVersion}

Check Update Reports Have Been Processed
    Directory Should Exist  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports
    ${files_in_processed_dir} =  List Files In Directory  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports
    Log  ${files_in_processed_dir}
    ${filesInUpdateVar} =  List Files In Directory  ${SOPHOS_INSTALL}/base/update/var/updatescheduler
    Log  ${filesInUpdateVar}
    ${UpdateReportCount} =  Count Files In Directory  ${SOPHOS_INSTALL}/base/update/var/updatescheduler  update_report[0-9]*.json

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check N Update Reports Processed  ${UpdateReportCount}

    Should Contain  ${files_in_processed_dir}[0]  update_report
    Should Not Contain  ${files_in_processed_dir}[0]  update_report.json
    Should Contain  ${filesInUpdateVar}   ${files_in_processed_dir}[0]

Check N Update Reports Processed
    [Arguments]  ${update_report_count}
    ${processed_file_count} =  Count Files In Directory  ${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports
    Should Be Equal As Integers  ${processed_file_count}  ${update_report_count}

Get Pid Of Process
    [Arguments]  ${process_name}
    ${result} =    Run Process  pidof  ${process_name}
    Should Be Equal As Integers    ${result.rc}    0   msg=${result.stderr}
    [Return]  ${result.stdout}

Check Mtr Reconnects To Management Agent After Upgrade
    Check Log Contains In Order
    ...  ${mdr_log_file}
    ...  mtr <> Plugin Finished.
    ...  mtr <> Entering the main loop
    ...  Received new policy
    ...  RevID of the new policy

Runtime Detections Plugin Is Installed And Running