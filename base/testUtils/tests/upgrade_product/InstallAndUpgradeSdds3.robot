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
Resource    ../GeneralUtilsResources.robot

Suite Setup      Upgrade Resources Suite Setup
Suite Teardown   Upgrade Resources Suite Teardown

Test Setup       Upgrade Resources Test Setup
Test Teardown    Upgrade Resources SDDS3 Test Teardown

Test Timeout  10 mins
Force Tags  LOAD9

*** Variables ***
${BaseEdrAndMtrAndAVDogfoodPolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT-1.xml
${BaseEdrAndMtrAndAVReleasePolicy}          ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_Release.xml
${BaseAndMtrVUTPolicy}                      ${GeneratedWarehousePolicies}/base_and_mtr_VUT.xml
${BaseEdrAndMtrAndAVVUTPolicy}              ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT.xml

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

${HealthyShsStatusXmlContents}              <item name="health" value="1" />
${GoodThreatHealthXmlContents}              <item name="threat" value="1" />
${BadThreatHealthXmlContents}               <item name="threat" value="2" />

*** Test Cases ***
Sul Downloader fails update if expected product missing from SUS
    [Teardown]    Upgrade Resources SDDS3 Test Teardown
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
    ...   Check Suldownloader Log Contains   Failed to connect to repository: Product doesn't match any suite: ServerProtectionLinux-Plugin-Fake

We Can Upgrade From Dogfood to VUT Without Unexpected Errors
    [Timeout]    12 minutes
    [Tags]    INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  MY_TEST

    &{expectedDogfoodVersions} =    Get Expected Versions    ${DOGFOOD_WAREHOUSE_ROOT}
    &{expectedVUTVersions} =    Get Expected Versions    ${VUT_WAREHOUSE_ROOT}

    Start Local Cloud Server    --initial-alc-policy    ${BaseEdrAndMtrAndAVDogfoodPolicy}
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

    ${handle}=    Start Local Dogfood SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080    https://localhost:8080
    Override LogConf File as Global Level    DEBUG

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2
    Check SulDownloader Log Contains   Running SDDS3 update

    # Update again to ensure we do not get a scheduled update later in the test run
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Trigger Update Now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120

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

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  15 secs
    ...  SHS Status File Contains    ${HealthyShsStatusXmlContents}

    Mark Watchdog Log
    Mark Managementagent Log
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}

    Trigger Update Now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  SHS Status File Contains  ${HealthyShsStatusXmlContents}
    Check SulDownloader Log Contains   Running SDDS3 update
    SHS Status File Contains  ${HealthyShsStatusXmlContents}
    SHS Status File Contains  ${GoodThreatHealthXmlContents}

    # Confirm that the warehouse flags supplement is installed when upgrading
    File Exists With Permissions  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  root  sophos-spl-group  -rw-r-----

    Check Watchdog Service File Has Correct Kill Mode

    Mark Known Upgrade Errors
    # If the policy comes down fast enough SophosMtr will not have started by the time MTR plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found

    # TODO LINUXDAR-2972 - expected till task is in released version
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 2] No such file or directory: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> utf8 write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry

    # This is expected because we are restarting the avplugin to enable debug logs, we need to make sure it occurs only once though
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> Exiting sophos_threat_detector with code: 15

    # When threat_detector is asked to shut down for upgrade it may have ongoing on-access scans that it has to abort
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/soapd.log  OnAccessImpl <> Aborting scan, scanner is shutting down

    Run Keyword And Expect Error  *
    ...     Check Log Contains String N times  ${SOPHOS_INSTALL}/plugins/av/log/av.log  av.log  Exiting sophos_threat_detector with code: 15  2

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Check Current Release With AV Installed Correctly
    Check SafeStore Database Has Not Changed    ${safeStoreDbDirBeforeUpgrade}    ${databaseContentBeforeUpgrade}    ${safeStorePasswordBeforeUpgrade}

    Wait For RuntimeDetections to be Installed
    Check RuntimeDetections Installed Correctly

    Check Expected Versions Against Installed Versions    &{expectedVUTVersions}

    Check Event Journaler Executable Running
    Check AV Plugin Permissions
    Check Update Reports Have Been Processed

    Wait Until Keyword Succeeds
    ...  180 secs
    ...  1 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}
    Check AV Plugin Can Scan Files
    Enable On Access Via Policy
    Check On Access Detects Threats

    SHS Status File Contains  ${HealthyShsStatusXmlContents}
    # Threat health returns to good after threat is cleaned up
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}

We Can Downgrade From VUT to Dogfood Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]   INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  BASE_DOWNGRADE

    &{expectedDogfoodVersions} =    Get Expected Versions    ${DOGFOOD_WAREHOUSE_ROOT}
    &{expectedVUTVersions} =    Get Expected Versions    ${VUT_WAREHOUSE_ROOT}
    ${expectBaseDowngrade} =  Second Version Is Lower  ${expectedVUTVersions["baseVersion"]}  ${expectedDogfoodVersions["baseVersion"]}

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

    ${handle}=    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080    https://localhost:8080
    Override LogConf File as Global Level    DEBUG

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains   Update success
    Check SulDownloader Log Contains    Running SDDS3 update

    # Update again to ensure we do not get a scheduled update later in the test run
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Trigger Update Now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120

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

    # Wait for successful update (all up to date) after downgrading
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Trigger Update Now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    200
    Check SulDownloader Log Contains   Running SDDS3 update

    Check for Management Agent Failing To Send Message To MTR And Check Recovery

    Mark Known Upgrade Errors
    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log  updatescheduler <> Update Service (sophos-spl-update.service) failed

    # TODO LINUXDAR-2972 - expected till task is in released version
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 2] No such file or directory: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> utf8 write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry

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
    ${new_sspl_update_uid} =     Get UID From Username    sophos-spl-updatescheduler
    Should Be Equal As Integers    ${sspl_user_uid}          ${new_sspl_user_uid}
    Should Be Equal As Integers    ${sspl_local_uid}         ${new_sspl_local_uid}
    Should Be Equal As Integers    ${sspl_update_uid}        ${new_sspl_update_uid}

    # Upgrade back to develop to check we can upgrade from a downgraded product
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
    Enable On Access Via Policy
    Check On Access Detects Threats

    # The query pack should have been re-installed
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  File Should Exist  ${Sophos_Scheduled_Query_Pack}

    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/av.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/soapd.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/sophos_threat_detector.log

    Wait Until Keyword Succeeds
    ...  180 secs
    ...  1 secs
    ...  SHS Status File Contains  ${HealthyShsStatusXmlContents}
    # Threat health returns to good after threat is cleaned up
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}

We Can Upgrade From Release to VUT Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  MY_TEST

    &{expectedReleaseVersions} =    Get Expected Versions    ${CURRENT_SHIPPING_WAREHOUSE_ROOT}
    &{expectedVUTVersions} =    Get Expected Versions    ${VUT_WAREHOUSE_ROOT}

    Start Local Cloud Server    --initial-alc-policy    ${BaseEdrAndMtrAndAVReleasePolicy}
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

    ${handle}=    Start Local Current Shipping SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080    https://localhost:8080
    Override LogConf File as Global Level    DEBUG

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2
    Check SulDownloader Log Contains    Running SDDS3 update

    # Update again to ensure we do not get a scheduled update later in the test run
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Trigger Update Now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120

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

    Wait Until Keyword Succeeds
    ...  210 secs
    ...  5 secs
    ...  SHS Status File Contains  ${HealthyShsStatusXmlContents}

    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Trigger Update Now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120
    Check SulDownloader Log Contains    Running SDDS3 update

    SHS Status File Contains  ${HealthyShsStatusXmlContents}
    SHS Status File Contains  ${GoodThreatHealthXmlContents}

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

    # TODO LINUXDAR-2972 - expected till task is in released version
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 2] No such file or directory: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> utf8 write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry

    #Required because release doesnt have libcrypto.so installed with the below plugins so they will encounter the error still
    #TODO LINUXDAR-7114 remove once SPL 2023.2 is released
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/eventjournaler/bin/eventjournaler died with exit code 127
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/responseactions/bin/responseactions died with exit code 127
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/plugins/mtr/bin/mtr died with exit code 127


    # This is expected because we are restarting the avplugin to enable debug logs, we need to make sure it occurs only once though
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/av.log  ScanProcessMonitor <> Exiting sophos_threat_detector with code: 15

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
    Check RuntimeDetections Installed Correctly

    Check AV Plugin Permissions
    Wait Until Keyword Succeeds
    ...  180 secs
    ...  1 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}
    Check AV Plugin Can Scan Files
    Enable On Access Via Policy
    Check On Access Detects Threats
    Check Update Reports Have Been Processed

    SHS Status File Contains  ${HealthyShsStatusXmlContents}
    # Threat health returns to good after threat is cleaned up
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}

We Can Downgrade From VUT to Release Without Unexpected Errors
    [Timeout]  10 minutes
    [Tags]   INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  BASE_DOWNGRADE

    &{expectedReleaseVersions} =    Get Expected Versions    ${CURRENT_SHIPPING_WAREHOUSE_ROOT}
    &{expectedVUTVersions} =    Get Expected Versions    ${VUT_WAREHOUSE_ROOT}
    ${expectBaseDowngrade} =  Second Version Is Lower  ${expectedVUTVersions["baseVersion"]}  ${expectedReleaseVersions["baseVersion"]}

    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

    ${handle}=    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080    https://localhost:8080
    Override LogConf File as Global Level    DEBUG

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains   Update success
    Check SulDownloader Log Contains    Running SDDS3 update

    # Update again to ensure we do not get a scheduled update later in the test run
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Trigger Update Now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120

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
    ${handle}=    Start Local Current Shipping SDDS3 Server
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
    ${ma_mark} =  mark_log_size  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log

    # Wait for successful update (all up to date) after downgrading
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Trigger Update Now
    wait_for_log_contains_from_mark  ${ma_mark}  Action ALC_action_FakeTime.xml sent to     15
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    200
    Check SulDownloader Log Contains    Running SDDS3 update

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

    # TODO LINUXDAR-2972 - expected till bugfix is in released version
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 2] No such file or directory: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  root <> utf8 write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry

    # When threat_detector is asked to shut down for upgrade it may have ongoing on-access scans that it has to abort
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/av/log/soapd.log  OnAccessImpl <> Aborting scan, scanner is shutting down

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

    Check EAP Release With AV Installed Correctly
    Check Expected Versions Against Installed Versions    &{expectedReleaseVersions}

    # Upgrade back to develop to check we can upgrade from a downgraded product
    Stop Local SDDS3 Server
    ${handle}=    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVVUTPolicy}

    Trigger Update Now

    Wait For Version Files to Update    &{expectedVUTVersions}

    Check AV Plugin Can Scan Files
    Enable On Access Via Policy
    Check On Access Detects Threats

    # The query pack should have been re-installed
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  5 secs
    ...  File Should Exist  ${Sophos_Scheduled_Query_Pack}

    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/av.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/soapd.log
    File Should Exist  ${SOPHOS_INSTALL}/plugins/av/log/downgrade-backup/sophos_threat_detector.log

    Wait Until Keyword Succeeds
    ...  180 secs
    ...  1 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  SHS Status File Contains  ${HealthyShsStatusXmlContents}

Sul Downloader Can Update Via Sdds3 Repository And Removes Local SDDS2 Cache
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}   --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_sdds2.json
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains   Update success
    Override LogConf File as Global Level  DEBUG
    Create Dummy Local SDDS2 Cache Files
    Check Local SDDS2 Cache Has Contents

    Create Local SDDS3 Override
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
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

    wait_for_log_contains_from_mark    ${sul_mark}    Update success    300
    Check Suldownloader Log Contains In Order    Update success    Purging local SDDS2 cache    Update success

    Check Local SDDS2 Cache Is Empty

    Check SulDownloader Log Contains String N Times   Generating the report file  2


SDDS3 updating respects ALC feature codes
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   Update success  2

    ${sul_mark} =  mark_log_size  ${SULDOWNLOADER_LOG_PATH}
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_CORE_only_feature_code.policy.xml  wait_for_policy=${True}

    wait_for_log_contains_from_mark    ${sul_mark}    Update success    80
    #core plugins should be installed
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/eventjournaler
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/runtimedetections
    #other plugins should be uninstalled
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/av
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/edr
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/liveresponse
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr
    check_log_does_not_contain_after_mark    ${SULDOWNLOADER_LOG_PATH}    Failed to remove path. Reason: Failed to delete file: /opt/sophos-spl/base/update/cache/sdds3primary/    ${sul_mark}

    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml  wait_for_policy=${True}
    Wait Until Keyword Succeeds
    ...   30 secs
    ...   5 secs
    ...   File Should Contain    ${UPDATE_CONFIG}    AV
    ${sul_mark} =  mark_log_size  ${SULDOWNLOADER_LOG_PATH}
    Trigger Update now
    wait_for_log_contains_from_mark  ${sul_mark}  Update success      120
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check Current Release With AV Installed Correctly
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/av

    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/edr
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/liveresponse
    Directory Should Exist   ${SOPHOS_INSTALL}/plugins/mtr


SDDS3 updating with changed unused feature codes do not change version
    Start Local Cloud Server
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains   Update success

    ${BaseVersionBeforeUpdate} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    ${EdrVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledEDRPluginVersionFile}
    ${LrVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledLRPluginVersionFile}
    ${AVVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledAVPluginVersionFile}
    ${RuntimeDetectionsVersionBeforeUpdate} =      Get Version Number From Ini File   ${InstalledRTDPluginVersionFile}
    ${EJVersionBeforeUpdate} =      Get Version Number From Ini File    ${InstalledEJPluginVersionFile}

    Override LogConf File as Global Level  DEBUG
    ${sul_mark} =  mark_log_size  ${SULDOWNLOADER_LOG_PATH}
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_fake_feature_codes_policy.xml  wait_for_policy=${True}
    wait_for_log_contains_from_mark  ${sul_mark}  Update success      120

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

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains   Update success

    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Trigger Update Now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120
    Check SulDownloader Log Contains String N Times   Generating the report file  2

    check_log_does_not_contain    extract_to  ${SOPHOS_INSTALL}/logs/base/suldownloader_sync.log  sync


Consecutive SDDS3 Updates Without Changes Should Not Trigger Additional Installations of Components
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains  Update success
    Check SulDownloader Log Contains     Running SDDS3 update

    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Trigger Update Now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    150
    check_log_does_not_contain_after_mark    ${SULDOWNLOADER_LOG_PATH}    Installing product    ${sul_mark}

    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Base-component' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-MDR' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-EDR' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-AV' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-liveresponse' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-RuntimeDetections' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-EventJournaler' is up to date.
    ${latest_report_result} =  Run Shell Process  ls ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report* -rt | cut -f 1 | tail -n1     OnError=failed to get last report file

    All Products In Update Report Are Up To Date  ${latest_report_result.stdout.strip()}


Schedule Query Pack Next Exists in SDDS3 and is Equal to Schedule Query Pack
    Start Local Cloud Server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    ${handle}=  Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}

    Configure And Run SDDS3 Thininstaller  0  https://localhost:8080   https://localhost:8080
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains   Update success
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


*** Keywords ***
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
