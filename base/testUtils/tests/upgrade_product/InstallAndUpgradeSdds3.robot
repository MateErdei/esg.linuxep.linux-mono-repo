*** Settings ***
Library    Collections
Library    OperatingSystem

Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/OSUtils.py
Library    ${LIBS_DIRECTORY}/SafeStoreUtils.py
Library    ${LIBS_DIRECTORY}/TelemetryUtils.py
Library    ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library    ${LIBS_DIRECTORY}/WarehouseUtils.py

Resource    ../av_plugin/AVResources.robot
Resource    ../event_journaler/EventJournalerResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../management_agent/ManagementAgentResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../runtimedetections_plugin/RuntimeDetectionsResources.robot
Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../telemetry/TelemetryResources.robot
Resource    ../watchdog/LogControlResources.robot
Resource    UpgradeResources.robot

Suite Setup      Suite Setup Without Ostia
Suite Teardown   Suite Teardown Without Ostia

Test Setup       Test Setup with Ostia
Test Teardown    Test Teardown With Ostia

Test Timeout  10 mins
Force Tags  LOAD9

*** Variables ***
${BaseEdrAndMtrAndAVDogfoodPolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT-1.xml
${BaseEdrAndMtrAndAVReleasePolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_Release.xml
${BaseAndMtrVUTPolicy}                      ${GeneratedWarehousePolicies}/base_and_mtr_VUT.xml
${BaseEdrAndMtrAndAVVUTPolicy}              ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT.xml
${BaseOnlyVUT_weekly_schedule_Policy}       ${GeneratedWarehousePolicies}/base_only_weeklyScheduleVUT.xml

${LocalWarehouseDir}                        ./local_warehouses
${SULDownloaderLog}                         ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${SULDownloaderSyncLog}                     ${SOPHOS_INSTALL}/logs/base/suldownloader_sync.log
${SULDownloaderLogDowngrade}                ${SOPHOS_INSTALL}/logs/base/downgrade-backup/suldownloader.log
${UpdateSchedulerLog}                       ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
${Sophos_Scheduled_Query_Pack}              ${SOPHOS_INSTALL}/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
${status_file}                              ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

${sdds2_primary}                            ${SOPHOS_INSTALL}/base/update/cache/primary
${sdds2_primary_warehouse}                  ${SOPHOS_INSTALL}/base/update/cache/primarywarehouse
${sdds3_primary}                            ${SOPHOS_INSTALL}/base/update/cache/sdds3primary
${sdds3_primary_repository}                 ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository

*** Test Cases ***
Sul Downloader fails update if expected product missing from SUS
    [Setup]    Test Setup
    [Teardown]    SDDS3 Test Teardown
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_FakePlugin.xml
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Create Local SDDS3 Override

    Register With Local Cloud Server

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   File Should Contain  ${UPDATE_CONFIG}     ServerProtectionLinux-Plugin-Fake

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   20 secs
    ...   5 secs
    ...   Check Suldownloader Log Contains   Failed to connect to repository: Package : ServerProtectionLinux-Plugin-Fake missing from warehouse

We Can Upgrade From Dogfood to VUT Without Unexpected Errors
    [Timeout]    10 minutes
    [Tags]    INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA

    &{expectedDogfoodVersions} =    Get Expected Versions    dogfood
    &{expectedVUTVersions} =    Get Expected Versions    vut

    Start Local Cloud Server    --initial-alc-policy    ${BaseEdrAndMtrAndAVDogfoodPolicy}

    ${handle}=    Start Local Dogfood SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080    https://localhost:8080    force_sdds3_post_install=${True}
    Override LogConf File as Global Level    DEBUG

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2
    Check SulDownloader Log Should Not Contain    Running in SDDS2 updating mode

    Check EAP Release With AV Installed Correctly
    Check SafeStore Installed Correctly
    ${safeStoreDbDirBeforeUpgrade} =    List Files In Directory    ${SAFESTORE_DB_DIR}
    ${safeStorePasswordBeforeUpgrade} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}
    ${databaseContentBeforeUpgrade} =    Get Contents of SafeStore Database
    Check Expected Versions Against Installed Versions    &{expectedDogfoodVersions}

    Stop Local SDDS3 Server
    ${handle}=    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Send ALC Policy And Prepare For Upgrade    ${BaseEdrAndMtrAndAVVUTPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File    ALC-1_policy.xml    ${BaseEdrAndMtrAndAVVUTPolicy}
    Wait Until Threat Detector Running

    ${HealthyShsStatusXmlContents} =  Set Variable    <item name="health" value="1" />
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  15 secs
    ...  SHS Status File Contains    ${HealthyShsStatusXmlContents}

    Mark Watchdog Log
    Mark Managementagent Log
    Start Process  tail -f ${SOPHOS_INSTALL}/logs/base/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true

    Trigger Update Now

    SHS Status File Contains  ${HealthyShsStatusXmlContents}
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    /tmp/preserve-sul-downgrade    Downgrade Log    Update success    2
    Check SulDownloader Log Should Not Contain    Running in SDDS2 updating mode
    SHS Status File Contains  ${HealthyShsStatusXmlContents}

    # Confirm that the warehouse flags supplement is installed when upgrading
    File Exists With Permissions  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  root  sophos-spl-group  -rw-r-----

    Check Watchdog Service File Has Correct Kill Mode

    Mark Known Upgrade Errors
    # If the policy comes down fast enough SophosMtr will not have started by the time MTR plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found

    #TODO LINUXDAR-2972 remove when this defect is fixed
    # Not an error should be a WARN instead, but it's happening on the EAP version so it's too late to change it now
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value

    # This is expected because we are restarting the avplugin to enable debug logs, we need to make sure it occurs only once though
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> Exiting sophos_threat_detector with code: 15

    # When threat_detector is asked to shut down for upgrade it may have ongoing on-access scans that it has to abort
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/soapd.log  OnAccessImpl <> Aborting scan, scanner is shutting down

    #TODO LINUXDAR-5140 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> failure in ConfigMonitor: pselect failed: Bad file descriptor
    Run Keyword And Expect Error  *
    ...     Check Log Contains String N  times ${SOPHOS_INSTALL}/plugins/av/log/av.log  av.log  Exiting sophos_threat_detector with code: 15  2

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Check Current Release With AV Installed Correctly
    Check SafeStore Database Has Not Changed    ${safeStoreDbDirBeforeUpgrade}    ${databaseContentBeforeUpgrade}    ${safeStorePasswordBeforeUpgrade}
    Wait For RuntimeDetections to be Installed
    Check Expected Versions Against Installed Versions    &{expectedVUTVersions}

    Check Event Journaler Executable Running
    Check AV Plugin Permissions
    Check Update Reports Have Been Processed

    Check AV Plugin Can Scan Files
    Check On Access Detects Threats
    SHS Status File Contains  ${HealthyShsStatusXmlContents}

We Can Downgrade From VUT to Dogfood Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]   INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA   BASE_DOWNGRADE

    &{expectedDogfoodVersions} =    Get Expected Versions    dogfood
    &{expectedVUTVersions} =    Get Expected Versions    vut
    ${expectBaseDowngrade} =  Second Version Is Lower  ${expectedVUTVersions["baseVersion"]}  ${expectedDogfoodVersions["baseVersion"]}

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}

    ${handle}=    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080    https://localhost:8080    force_sdds3_post_install=${True}
    Override LogConf File as Global Level    DEBUG

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2
    Check SulDownloader Log Should Not Contain    Running in SDDS2 updating mode

    Check Current Release With AV Installed Correctly
    ${safeStoreDbDirBeforeUpgrade} =    List Files In Directory    ${SAFESTORE_DB_DIR}
    ${safeStorePasswordBeforeUpgrade} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}
    ${databaseContentBeforeUpgrade} =    Get Contents of SafeStore Database
    Check Expected Versions Against Installed Versions    &{expectedVUTVersions}

    Directory Should Not Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup
    # Products that should be uninstalled after downgrade
    Should Exist  ${InstalledLRPluginVersionFile}
    # The query pack should have been installed with EDR VUT
    Should Exist  ${Sophos_Scheduled_Query_Pack}

    ${sspl_user_uid} =       Get UID From Username    sophos-spl-user
    ${sspl_local_uid} =      Get UID From Username    sophos-spl-local
    ${sspl_network_uid} =    Get UID From Username    sophos-spl-network
    ${sspl_update_uid} =     Get UID From Username    sophos-spl-updatescheduler

    # Changing the policy here will result in an automatic update
    # Note when downgrading from a release with live response to a release without live response
    # results in a second update.
    Stop Local SDDS3 Server
    ${handle}=    Start Local Dogfood SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVDogfoodPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseEdrAndMtrAndAVDogfoodPolicy}

    Mark Watchdog Log
    Mark Managementagent Log
    Start Process  tail -fn0 ${SOPHOS_INSTALL}/logs/base/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    /tmp/preserve-sul-downgrade    Downgrade Log    Update success    1
    Run Keyword If  ${ExpectBaseDowngrade}    Check Log Contains    Preparing ServerProtectionLinux-Base-component for downgrade    ${SULDownloaderLogDowngrade}    backedup suldownloader log

    Trigger Update Now
    # Wait for successful update (all up to date) after downgrading
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  10 secs
    ...  Check SulDownloader Log Contains String N Times    Update success    1
    Check SulDownloader Log Should Not Contain    Running in SDDS2 updating mode

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

    # When threat_detector is asked to shut down for upgrade it may have ongoing on-access scans that it has to abort
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/soapd.log  OnAccessImpl <> Aborting scan, scanner is shutting down

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Check EAP Release With AV Installed Correctly
    Check SafeStore Installed Correctly
    Wait Until Keyword Succeeds
    ...    120 secs
    ...    10 secs
    ...    Check Log Contains    Successfully restored old SafeStore database    /tmp/preserve-sul-downgrade    Downgrade Log
    Check SafeStore Database Has Not Changed    ${safeStoreDbDirBeforeUpgrade}    ${databaseContentBeforeUpgrade}    ${safeStorePasswordBeforeUpgrade}
    Check Expected Versions Against Installed Versions    &{expectedDogfoodVersions}

    # Check users haven't been removed and added back
    ${new_sspl_user_uid} =       Get UID From Username    sophos-spl-user
    ${new_sspl_local_uid} =      Get UID From Username    sophos-spl-local
    ${new_sspl_network_uid} =    Get UID From Username    sophos-spl-network
    ${new_sspl_update_uid} =     Get UID From Username    sophos-spl-updatescheduler
    Should Be Equal As Integers    ${sspl_user_uid}          ${new_sspl_user_uid}
    Should Be Equal As Integers    ${sspl_local_uid}         ${new_sspl_local_uid}
    Should Be Equal As Integers    ${sspl_network_uid}       ${new_sspl_network_uid}
    Should Be Equal As Integers    ${sspl_update_uid}        ${new_sspl_update_uid}

    # Upgrade back to master to check we can upgrade from a downgraded product
    Stop Local SDDS3 Server
    ${handle}=    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVVUTPolicy}

    Trigger Update Now

    Wait For Version Files to Update    &{expectedVUTVersions}
    Toggle SafeStore Flag in MCS Policy    ${True}
    Check SafeStore Installed Correctly
    Check SafeStore Database Has Not Changed    ${safeStoreDbDirBeforeUpgrade}    ${databaseContentBeforeUpgrade}    ${safeStorePasswordBeforeUpgrade}

    Check AV Plugin Can Scan Files
    Check On Access Detects Threats

    # The query pack should have been re-installed
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  File Should Exist  ${Sophos_Scheduled_Query_Pack}

We Can Upgrade From Release to VUT Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA

    &{expectedReleaseVersions} =    Get Expected Versions    current_shipping
    &{expectedVUTVersions} =    Get Expected Versions    vut

    Start Local Cloud Server    --initial-alc-policy    ${BaseEdrAndMtrAndAVReleasePolicy}

    ${handle}=    Start Local Release SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080    https://localhost:8080    force_sdds3_post_install=${True}
    Override LogConf File as Global Level    DEBUG

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2
    Check SulDownloader Log Should Not Contain    Running in SDDS2 updating mode

    Check EAP Release With AV Installed Correctly
    Check Expected Versions Against Installed Versions    &{expectedReleaseVersions}

    Stop Local SDDS3 Server
    ${handle}=    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVVUTPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check Policy Written Match File  ALC-1_policy.xml  ${BaseEdrAndMtrAndAVVUTPolicy}
    Wait Until Threat Detector Running

    ${HealthyShsStatusXmlContents} =  Set Variable  <item name="health" value="1" />
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  SHS Status File Contains  ${HealthyShsStatusXmlContents}

    Start Process  tail -f ${SOPHOS_INSTALL}/logs/base/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    /tmp/preserve-sul-downgrade    Downgrade Log    Update success    2
    Check SulDownloader Log Should Not Contain    Running in SDDS2 updating mode

    SHS Status File Contains  ${HealthyShsStatusXmlContents}

    # Confirm that the warehouse flags supplement is installed when upgrading
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

    # This is expected because we are restarting the avplugin to enable debug logs, we need to make sure it occurs only once though
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> Exiting sophos_threat_detector with code: 15
    #TODO LINUXDAR-5140 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> failure in ConfigMonitor: pselect failed: Bad file descriptor
    Run Keyword And Expect Error  *
    ...     Check Log Contains String N  times ${SOPHOS_INSTALL}/plugins/av/log/av.log  av.log  Exiting sophos_threat_detector with code: 15  2

    # When threat_detector is asked to shut down for upgrade it may have ongoing on-access scans that it has to abort
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/soapd.log  OnAccessImpl <> Aborting scan, scanner is shutting down

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Check Current Release With AV Installed Correctly
    Check Expected Versions Against Installed Versions    &{expectedVUTVersions}

    Check Event Journaler Executable Running
    Wait For RuntimeDetections to be Installed
    Check AV Plugin Permissions

    Check AV Plugin Can Scan Files
    Check On Access Detects Threats
    Check Update Reports Have Been Processed

    SHS Status File Contains  ${HealthyShsStatusXmlContents}

We Can Downgrade From VUT to Release Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]   INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA   BASE_DOWNGRADE

    &{expectedReleaseVersions} =    Get Expected Versions    current_shipping
    &{expectedVUTVersions} =    Get Expected Versions    vut
    ${expectBaseDowngrade} =  Second Version Is Lower  ${expectedVUTVersions["baseVersion"]}  ${expectedReleaseVersions["baseVersion"]}

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}

    ${handle}=    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080    https://localhost:8080    force_sdds3_post_install=${True}
    Override LogConf File as Global Level    DEBUG

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2
    Check SulDownloader Log Should Not Contain    Running in SDDS2 updating mode

    Check Current Release With AV Installed Correctly
    Check Expected Versions Against Installed Versions    &{expectedVUTVersions}

    Directory Should Not Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup
    # Products that should be uninstalled after downgrade
    Should Exist  ${InstalledLRPluginVersionFile}
    # The query pack should have been installed with EDR VUT
    Should Exist  ${Sophos_Scheduled_Query_Pack}

    # Changing the policy here will result in an automatic update
    # Note when downgrading from a release with live response to a release without live response
    # results in a second update.
    Stop Local SDDS3 Server
    ${handle}=    Start Local Release SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
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
    Start Process  tail -fn0 ${SOPHOS_INSTALL}/logs/base/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    /tmp/preserve-sul-downgrade    Downgrade Log    Update success    1
    Run Keyword If  ${ExpectBaseDowngrade}    Check Log Contains    Preparing ServerProtectionLinux-Base-component for downgrade    ${SULDownloaderLogDowngrade}  backedup suldownloader log

    Trigger Update Now
    # Wait for successful update (all up to date) after downgrading
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  10 secs
    ...  Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log    SulDownloader Log    Update success    1
    Check SulDownloader Log Should Not Contain    Running in SDDS2 updating mode

    Wait Until Keyword Succeeds
    ...   60 secs
    ...   10 secs
    ...   Should Not Exist  ${SOPHOS_INSTALL}/base/mcs/action/testfile

    Check for Management Agent Failing To Send Message To MTR And Check Recovery

    Mark Known Upgrade Errors
    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log  updatescheduler <> Update Service (sophos-spl-update.service) failed
    #TODO LINUXDAR-2972 remove when this defect is fixed
    # Not an error should be a WARN instead, but it's happening on the EAP version so it's too late to change it now
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    #TODO LINUXDAR-5140 remove when this defect is closed
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> failure in ConfigMonitor: pselect failed: Bad file descriptor

    # When threat_detector is asked to shut down for upgrade it may have ongoing on-access scans that it has to abort
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/soapd.log  OnAccessImpl <> Aborting scan, scanner is shutting down

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Check EAP Release With AV Installed Correctly
    Check Expected Versions Against Installed Versions    &{expectedReleaseVersions}

    # Upgrade back to master to check we can upgrade from a downgraded product
    Stop Local SDDS3 Server
    ${handle}=    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVVUTPolicy}

    Trigger Update Now

    Wait For Version Files to Update    &{expectedVUTVersions}

    # TODO: Uncomment below once On-access has been released
    #Check AV Plugin Can Scan Files
    #Check On Access Detects Threats

    # The query pack should have been re-installed
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  File Should Exist  ${Sophos_Scheduled_Query_Pack}

Sul Downloader Can Update Via Sdds3 Repository And Removes Local SDDS2 Cache
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2
    Override LogConf File as Global Level  DEBUG
    Check Local SDDS2 Cache Has Contents

    Create Local SDDS3 Override
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   Directory Should Exist   ${sdds3_primary_repository}
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Directory Should Exist   ${sdds3_primary_repository}/suite
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   2 secs
    ...   Directory Should Exist   ${sdds3_primary_repository}/package
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   2 secs
    ...   Directory Should Exist   ${sdds3_primary_repository}/supplement
    Wait Until Keyword Succeeds
    ...   240 secs
    ...   10 secs
    ...   Directory Should Exist   ${sdds3_primary}/ServerProtectionLinux-Base-component/

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  3
    Check Suldownloader Log Contains In Order    Update success    Purging local SDDS2 cache    Update success

    Check Local SDDS2 Cache Is Empty

    Check SulDownloader Log Contains String N Times   Generating the report file  3


SDDS3 updating respects ALC feature codes
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2
    Check Local SDDS2 Cache Has Contents

    Create Local SDDS3 Override
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_CORE_only_feature_code.policy.xml  wait_for_policy=${True}
    Mark Sul Log
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  3
    Check SulDownloader Log Contains String N Times   Generating the report file  3
    #core plugins should be installed
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/eventjournaler
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/runtimedetections
    #other plugins should be uninstalled
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/av
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/edr
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/liveresponse
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr
    Check Marked Sul Log Does Not Contain   Failed to remove path. Reason: Failed to delete file: /opt/sophos-spl/base/update/cache/sdds3primary/
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml  wait_for_policy=${True}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  4
    Check SulDownloader Log Contains String N Times   Generating the report file  4
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/av
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/edr
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/liveresponse
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/mtr


SDDS3 updating with changed unused feature codes do not change version
    Start Local Cloud Server
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080  force_sdds3_post_install=${True}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2

    ${BaseVersionBeforeUpdate} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    ${EdrVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    ${LrVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledLRPluginVersionFile}
    ${AVVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    ${RuntimeDetectionsVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledRTDPluginVersionFile}
    ${EJVersionBeforeUpdate} =      Get Version Number From Ini File    ${InstalledEJPluginVersionFile}

    Override LogConf File as Global Level  DEBUG
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_fake_feature_codes_policy.xml  wait_for_policy=${True}
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  3
    Check SulDownloader Log Contains String N Times   Generating the report file  3
    #TODO once defect LINUXDAR-4592 is done check here that no plugins are reinstalled as well
    ${BaseVersionAfterUpdate} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrVersionAfterUpdate} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    ${EdrVersionAfterUpdate} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    ${LrVersionAfterUpdate} =      Get Version Number From Ini File   ${InstalledLRPluginVersionFile}
    ${AVVersionAfterUpdate} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    ${RuntimeDetectionsVersionAfterUpdate} =      Get Version Number From Ini File   ${InstalledRTDPluginVersionFile}
    ${EJVersionAfterUpdate} =      Get Version Number From Ini File    ${InstalledEJPluginVersionFile}
    Should Be Equal As Strings  ${RuntimeDetectionsVersionBeforeUpdate}  ${RuntimeDetectionsVersionAfterUpdate}
    Should Be Equal As Strings  ${MtrVersionBeforeUpdate}  ${MtrVersionAfterUpdate}
    Should Be Equal As Strings  ${EdrVersionBeforeUpdate}  ${EdrVersionAfterUpdate}
    Should Be Equal As Strings  ${LrVersionBeforeUpdate}  ${LrVersionAfterUpdate}
    Should Be Equal As Strings  ${AVVersionBeforeUpdate}  ${AVVersionAfterUpdate}
    Should Be Equal As Strings  ${EJVersionBeforeUpdate}  ${EJVersionAfterUpdate}
    Should Be Equal As Strings  ${BaseVersionBeforeUpdate}  ${BaseVersionAfterUpdate}


SDDS3 updating when warehouse files have not changed does not extract the zip files
    Start Local Cloud Server
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080  force_sdds3_post_install=${True}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  3
    Check SulDownloader Log Contains String N Times   Generating the report file  3

    check_log_does_not_contain    extract_to  ${SOPHOS_INSTALL}/logs/base/suldownloader_sync.log  sync


We can Install With SDDS3 Perform an SDDS2 Initial Update With SDDS3 Flag False Then Update Using SDDS3 When Flag Turns True
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2

    #check first two updates are SDDS3 (install) followed by SDDS2 update (5th line only appears in sdds2 mode)
    Check SulDownloader Log Contains String N Times   Running in SDDS3 updating mode  1
    Check Suldownloader Log Contains In Order
    ...  Logger suldownloader configured for level: INFO
    ...  Running in SDDS3 updating mode
    ...  Update success
    ...  Logger suldownloader configured for level: INFO
    ...  Products downloaded and synchronized with warehouse
    ...  Update success

    #Don't force SDDS3 as we want to use flags to trigger it.
    Create Local SDDS3 Override  USE_SDDS3_OVERRIDE=false

    #prevent mcs overwriting flags
    File Should Contain  ${UPDATE_CONFIG}     "useSDDS3": false
    Overwrite MCS Flags File  {"sdds3.enabled": true}
    Wait Until Keyword Succeeds
    ...     10s
    ...     2s
    ...     File Should Contain  ${UPDATE_CONFIG}     "useSDDS3": true
    Mark Sul Log
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Marked Sul Log Contains    Running in SDDS3 updating mode
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  3
    Check SulDownloader Log Contains String N Times   Generating the report file  3
    Check SulDownloader Log Contains String N Times   Running in SDDS3 updating mode  2


We can Install With SDDS3 Perform an SDDS3 Initial Update With SDDS3 Flag True Then Update Using SDDS2 When Flag Turns False
    [Teardown]    Test Teardown With Ostia And Fake Cloud MCS Flag Override
    ${desired_flags} =     Catenate    SEPARATOR=\n
    ...  {
    ...    "livequery.network-tables.available" : true,
    ...    "endpoint.flag2.enabled" : false,
    ...    "endpoint.flag3.enabled" : false,
    ...    "jwt-token.available" : true,
    ...    "mcs.v2.data_feed.available": true,
    ...    "sdds3.enabled": true
    ...  }

    Create File  /tmp/mcs_flags  ${desired_flags}
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2

    #check first two updates are SDDS3 (installation + initial update)
    Check SulDownloader Log Contains String N Times   Running in SDDS3 updating mode  2
    Check Suldownloader Log Contains In Order
    ...  Logger suldownloader configured for level: INFO
    ...  Running in SDDS3 updating mode
    ...  Update success
    ...  Logger suldownloader configured for level: INFO
    ...  Running in SDDS3 updating mode
    ...  Update success

    #prevent mcs overwriting flags
    File Should Contain  ${UPDATE_CONFIG}     "useSDDS3": true
    Overwrite MCS Flags File  {"sdds3.enabled": false}
    Wait Until Keyword Succeeds
    ...     10s
    ...     2s
    ...     File Should Contain  ${UPDATE_CONFIG}     "useSDDS3": false
    Mark Sul Log
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check Marked Sul Log Does Not Contain    Running in SDDS3 updating mode
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  3
    Check SulDownloader Log Contains String N Times   Generating the report file  3
    Check SulDownloader Log Contains String N Times   Running in SDDS3 updating mode  2


Consecutive SDDS3 Updates Without Changes Should Not Trigger Additional Installations of Components
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080   force_sdds3_post_install=${True}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2
    Check SulDownloader Log Contains String N Times   Running in SDDS3 updating mode  2

    Mark Sul Log
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Marked Sul Log Contains   Update success
    Check Marked Sul Log Does Not Contain  Installing product

    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Base-component' is up to date.
    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Plugin-MDR' is up to date.
    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Plugin-EDR' is up to date.
    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Plugin-AV' is up to date.
    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Plugin-liveresponse' is up to date.
    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Plugin-RuntimeDetections' is up to date.
    Check Marked Sul Log Contains   Downloaded Product line: 'ServerProtectionLinux-Plugin-EventJournaler' is up to date.
    ${latest_report} =  Run Shell Process  ls ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report* -rt | cut -f 1 | tail -n1  shell=${True}
    All Products In Update Report Are Up To Date  ${latest_report.strip()}


During Transition From SDDS3 to SDDS2 SDDS3 Cache Is Removed Before Downloading SDDS2 Files
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains    Update success    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log

    Override LogConf File as Global Level  DEBUG
    Wait Until Keyword Succeeds
    ...   60 secs
    ...   5 secs
    ...   Check Local SDDS3 Cache Has Contents

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...  150 secs
    ...  10 secs
    ...  Check Suldownloader Log Contains In Order    Update success    Purging local SDDS3 cache    Update success

    Check Local SDDS3 Cache Is Empty
    Check Local SDDS2 Cache Has Contents


Schedule Query Pack Next Exists in SDDS3 and is Equal to Schedule Query Pack
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2
    Create Local SDDS3 Override
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   60 secs
    ...   5 secs
    ...   Check Local SDDS3 Cache Has Contents

    Wait Until Keyword Succeeds
    ...  300 secs
    ...  10 secs
    ...  Directory Should Exist  ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  10 secs
    ...  Directory Should Exist  ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  10 secs
    ...  Directory Should Exist  ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack_next

    ${scheduled_query_pack} =             Get File    ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack/sophos-scheduled-query-pack.conf
    ${scheduled_query_pack_next} =        Get File    ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack_next/sophos-scheduled-query-pack.conf
    Should Be Equal As Strings            ${scheduled_query_pack}    ${scheduled_query_pack_next}

    ${scheduled_query_pack_mtr} =         Get File    ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf
    ${scheduled_query_pack_next_mtr} =    Get File    ${sdds3_primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf
    Should Be Equal As Strings            ${scheduled_query_pack_mtr}    ${scheduled_query_pack_next_mtr}


SDDS3 Mechanism Is Updated in UpdateScheduler Telemetry After Successful Update To SDDS3
    Cleanup Telemetry Server
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2
    Set Log Level For Component And Reset and Return Previous Log  updatescheduler   DEBUG

    Wait Until Keyword Succeeds
    ...    30 secs
    ...    5 secs
    ...    Check UpdateScheduler Log Contains String N Times    Sending status to Central    2

    Prepare To Run Telemetry Executable With HTTPS Protocol    port=6443
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log File   ${TELEMETRY_OUTPUT_JSON}
    Check Update Scheduler Telemetry Json Is Correct  ${telemetryFileContents}  0   sddsid=av_user_vut  set_edr=True  set_av=True  most_recent_update_successful=True  sdds_mechanism=SDDS2

    Overwrite MCS Flags File    {"sdds3.enabled": true}
    Wait Until Keyword Succeeds
    ...     10s
    ...     2s
    ...     File Should Contain  ${UPDATE_CONFIG}     "useSDDS3": true
    Check Updatescheduler Log Contains    Received sdds3.enabled flag value: 1

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    200 secs
    ...    10 secs
    ...    Check Suldownloader Log Contains In Order    Update success    Running in SDDS3 updating mode   Generating the report file
    Wait Until Keyword Succeeds
    ...    30 secs
    ...    5 secs
    ...    Check UpdateScheduler Log Contains String N Times    Sending status to Central    4

    Cleanup Telemetry Server
    Prepare To Run Telemetry Executable With HTTPS Protocol    port=6443
    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    Log File   ${TELEMETRY_OUTPUT_JSON}
    Check Update Scheduler Telemetry Json Is Correct  ${telemetryFileContents}  0   sddsid=av_user_vut  set_edr=True  set_av=True  most_recent_update_successful=True  sdds_mechanism=SDDS3
    Cleanup Telemetry Server


*** Keywords ***
Test Setup With Ostia
    Test Setup
    Setup Ostia Warehouse Environment

Test Teardown With Ostia
    Stop Local SDDS3 Server
    Teardown Ostia Warehouse Environment
    Test Teardown

Test Teardown With Ostia And Fake Cloud MCS Flag Override
    Test Teardown With Ostia
    Remove File  /tmp/mcs_flags

Create Dummy Local SDDS2 Cache Files
    Create File         ${sdds2_primary}/1
    Create Directory    ${sdds2_primary}/2
    Create File         ${sdds2_primary}/2f
    Create Directory    ${sdds2_primary}/2d
    Create File         ${sdds2_primary_warehouse}/1
    Create Directory    ${sdds2_primary_warehouse}/2
    Create File         ${sdds2_primary_warehouse}/2f
    Create Directory    ${sdds2_primary_warehouse}/2d

Check Local SDDS2 Cache Is Empty
    Directory Should Be Empty    ${sdds2_primary}
    Directory Should Be Empty    ${sdds2_primary_warehouse}

Check Local SDDS3 Cache Is Empty
    Directory Should Be Empty    ${sdds3_primary}
    Directory Should Be Empty    ${sdds3_primary_repository}

Check Local SDDS2 Cache Has Contents
    Directory Should Not Be Empty    ${sdds2_primary}
    Directory Should Not Be Empty    ${sdds2_primary_warehouse}

Check Local SDDS3 Cache Has Contents
    Directory Should Not Be Empty    ${sdds3_primary}
    Directory Should Not Be Empty    ${sdds3_primary_repository}
