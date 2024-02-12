*** Settings ***
Library    Collections
Library    OperatingSystem
Library    String

Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/LiveQueryUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/MCSRouter.py
Library    ${COMMON_TEST_LIBS}/OSUtils.py
Library    ${COMMON_TEST_LIBS}/SafeStoreUtils.py
Library    ${COMMON_TEST_LIBS}/ThinInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/UpdateSchedulerHelper.py
Library    ${COMMON_TEST_LIBS}/UpgradeUtils.py
Library    ${COMMON_TEST_LIBS}/WarehouseUtils.py

Resource    PluginResources.robot
Resource    ProductResources.robot
Resource    ${COMMON_TEST_ROBOT}/CommonErrorMarkers.robot
Resource    ${COMMON_TEST_ROBOT}/EventJournalerResources.robot
Resource    ${COMMON_TEST_ROBOT}/GeneralUtilsResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Suite Setup      Upgrade Resources Suite Setup

Test Setup       Install And Upgrade Test Setup
Test Teardown    Upgrade Resources SDDS3 Test Teardown

Test Timeout  10 mins
Force Tags    TAP_PARALLEL2


*** Variables ***
${SULDownloaderLog}                   ${BASE_LOGS_DIR}/suldownloader.log
${Sophos_Scheduled_Query_Pack}        ${EDR_DIR}/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
${updateConfig}                       ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json

${SDDS3Primary}                       ${SOPHOS_INSTALL}/base/update/cache/sdds3primary
${SDDS3PrimaryRepository}             ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository

${HealthyShsStatusXmlContents}        <item name="health" value="1" />
${GoodThreatHealthXmlContents}        <item name="threat" value="1" />
${GoodIsolationXmlContents}           <item name="admin" value="1" />

# Thin installer appends sophos-spl to the argument
${CUSTOM_INSTALL_DIRECTORY}    /home/parent/sophos-spl
${CUSTOM_INSTALL_DIRECTORY_ARG}    /home/parent
${RPATHCheckerLog}                          /tmp/rpath_checker.log

*** Test Cases ***
Sul Downloader fails update if expected product missing from SUS
    start_local_cloud_server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy/ALC_policy_FakePlugin.xml
    Start Local SDDS3 Server

    configure_and_run_SDDS3_thininstaller    ${18}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}
    check_thininstaller_log_contains    Failed to connect to repository: Product doesn't match any suite: ServerProtectionLinux-Plugin-Fake

We Can Upgrade From Dogfood to VUT Without Unexpected Errors
    &{expectedDogfoodVersions} =    Get Expected Versions For Recommended Tag    ${DOGFOOD_WAREHOUSE_REPO_ROOT}    ${DOGFOOD_LAUNCH_DARKLY}
    &{expectedVUTVersions} =    Get Expected Versions For Recommended Tag    ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}

    ${policy} =  Create Core Policy To enable on Access  revisionId=EnableOnAccessPolicy
    start_local_cloud_server  --initial-core-policy  ${policy}

    Start Local Dogfood SDDS3 Server
    ${rtd_mark} =    mark_log_size    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log
    ${all_plugins_logs_marks} =    Mark All Plugin Logs
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}    sophos_log_level=DEBUG
    Wait For Plugins To Be Ready    log_marks=${all_plugins_logs_marks}

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent    1

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_suldownloader_log_contains_string_n_times    Update success    1
    check_suldownloader_log_contains    Running SDDS3 update

    # Update again to ensure we do not get a scheduled update later in the test run
    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120

    Check Current Shipping Installed Correctly    ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    Run Keyword Unless
    ...  ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    ...  wait_for_log_contains_from_mark    ${rtd_mark}    Analytics started processing telemetry    ${RTD_STARTUP_TIMEOUT}
    ${safeStoreDbDirBeforeUpgrade} =    List Files In Directory    ${SAFESTORE_DB_DIR}
    ${safeStorePasswordBeforeUpgrade} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}
    ${databaseContentBeforeUpgrade} =    get_contents_of_safestore_database
    Check Expected Versions Against Installed Versions    &{expectedDogfoodVersions}

    Stop Local SDDS3 Server

    # Upgrading to VUT
    ${rtd_mark} =    mark_log_size    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log
    ${watchdog_pid_before_upgrade}=     Run Process    pgrep    -f    sophos_watchdog
    Start Local SDDS3 Server

    Wait Until Threat Detector Running

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  15 secs
    ...  SHS Status File Contains    ${HealthyShsStatusXmlContents}

    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    ${all_plugins_logs_mark} =    Mark All Plugin Logs
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120
    Wait For Plugins To Be Ready    log_marks=${all_plugins_logs_mark}

    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  SHS Status File Contains    ${HealthyShsStatusXmlContents}
    check_suldownloader_log_contains    Running SDDS3 update
    SHS Status File Contains    ${HealthyShsStatusXmlContents}
    SHS Status File Contains    ${GoodThreatHealthXmlContents}

    # Confirm that the warehouse flags supplement is installed when upgrading
    file_exists_with_permissions    ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  root  sophos-spl-group  -rw-r-----
    check_watchdog_service_file_has_correct_kill_mode

    Mark Known Upgrade Errors
    # TODO LINUXDAR-2972 - expected till task is in released version
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 2] No such file or directory: '/opt/sophos-spl/tmp/policy/flags.json'
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  root <> utf8 write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    mark_expected_error_in_log  ${AV_DIR}/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry
    # This is expected because we are restarting the avplugin to enable debug logs, we need to make sure it occurs only once though
    mark_expected_error_in_log  ${AV_DIR}/log/av.log  ScanProcessMonitor <> Exiting sophos_threat_detector with code: 15
    # When threat_detector is asked to shut down for upgrade it may have ongoing on-access scans that it has to abort
    mark_expected_error_in_log  ${AV_DIR}/log/soapd.log  OnAccessImpl <> Aborting scan, scanner is shutting down
    # ZMQ error that is still in dogfood
    mark_expected_error_in_log  ${AV_DIR}/log/av.log  No incoming data on ZMQ socket from getReply in BaseServiceAPI after

    IF    ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
        Mark Expected Error In Log    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log    runtimedetections <> supervisor entering dormant mode due to error
        Mark Expected Error In Log    ${BASE_LOGS_DIR}/watchdog.log    ProcessMonitoringImpl <> /opt/sophos-spl/plugins/runtimedetections/bin/runtimedetections died with exit code 1
        Mark Expected Error In Log    ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log    managementagent <> Failure on sending message to runtimedetections. Reason: No incoming data on ZMQ socket from getReply in PluginProxy
        Mark Expected Error In Log    ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log    managementagent <> Failure on sending message to runtimedetections. Reason: No incoming data on ZMQ socket from getReply in PluginProxy
    END

    Run Keyword And Expect Error  *
    ...     check_log_contains_string_n_times  ${AV_DIR}/log/av.log  av.log  Exiting sophos_threat_detector with code: 15  2

    check_all_product_logs_do_not_contain_error
    check_all_product_logs_do_not_contain_critical

    Check VUT Installed Correctly     ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    Run Keyword Unless
    ...  ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    ...  wait_for_log_contains_from_mark    ${rtd_mark}    Analytics started processing telemetry    ${RTD_STARTUP_TIMEOUT}
    Check SafeStore Database Has Not Changed    ${safeStoreDbDirBeforeUpgrade}    ${databaseContentBeforeUpgrade}    ${safeStorePasswordBeforeUpgrade}
    Check Expected Versions Against Installed Versions    &{expectedVUTVersions}

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

    ${watchdog_pid_after_upgrade}=     Run Process    pgrep    -f    sophos_watchdog
    Should Not Be Equal As Integers    ${watchdog_pid_before_upgrade.stdout}    ${watchdog_pid_after_upgrade.stdout}
    Run Keyword Unless    ${KERNEL_VERSION_TOO_OLD_FOR_RTD}    Check RuntimeDetections Installed Correctly


Install VUT and Check RPATH of Every Binary
    [Timeout]    3 minutes

    start_local_cloud_server

    Start Local SDDS3 Server

    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}
    Override LogConf File as Global Level    DEBUG

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_suldownloader_log_contains   Update success
    check_suldownloader_log_contains    Running SDDS3 update

    # Update again to ensure we do not get a scheduled update later in the test run
    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120

    # Run Process can hang with large outputs which RPATHChecker.sh can have
    # So redirecting RPATHChecker.sh output to a file rather than console
    Run Process    chmod    +x    ${SUPPORT_FILES}/RPATHChecker.sh
    ${result} =    Run Process    ${SUPPORT_FILES}/RPATHChecker.sh    shell=true    stdout=${RPATHCheckerLog}
    log file    ${RPATHCheckerLog}
    Log    Output of RPATHChecker.sh written to ${RPATHCheckerLog}    console=True
    IF    $result.rc != 0
        IF    $result.rc == 1
            Fail    Insecure RPATH(s) found,
        ELSE
            Fail    ERROR: Unknown return code
        END
    END

We Can Downgrade From VUT to Dogfood Without Unexpected Errors
    &{expectedDogfoodVersions} =    Get Expected Versions For Recommended Tag    ${DOGFOOD_WAREHOUSE_REPO_ROOT}    ${DOGFOOD_LAUNCH_DARKLY}
    &{expectedVUTVersions} =    Get Expected Versions For Recommended Tag    ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}
    ${expectBaseDowngrade} =  second_version_is_lower  ${expectedVUTVersions["baseVersion"]}  ${expectedDogfoodVersions["baseVersion"]}

    start_local_cloud_server
    # Enable OnAccess
    Send Core Policy To enable on Access
    ${rtd_mark} =    mark_log_size    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log
    Start Local SDDS3 Server

    ${all_plugins_logs_marks} =    Mark All Plugin Logs
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}    sophos_log_level=DEBUG
    Wait For Plugins To Be Ready    log_marks=${all_plugins_logs_marks}

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_suldownloader_log_contains   Update success
    check_suldownloader_log_contains    Running SDDS3 update

    # Update again to ensure we do not get a scheduled update later in the test run
    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120

    Check VUT Installed Correctly     ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    Run Keyword Unless
    ...  ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    ...  wait_for_log_contains_from_mark    ${rtd_mark}    Analytics started processing telemetry    ${RTD_STARTUP_TIMEOUT}
    ${safeStoreDbDirBeforeUpgrade} =    List Files In Directory    ${SAFESTORE_DB_DIR}
    ${safeStorePasswordBeforeUpgrade} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}
    ${databaseContentBeforeUpgrade} =    get_contents_of_safestore_database
    Check Expected Versions Against Installed Versions    &{expectedVUTVersions}

    ${sspl_user_uid} =       get_uid_from_username    sophos-spl-user
    ${sspl_local_uid} =      get_uid_from_username    sophos-spl-local
    ${sspl_update_uid} =     get_uid_from_username    sophos-spl-updatescheduler

    Stop Local SDDS3 Server
    ${rtd_mark} =    mark_log_size    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log
    ${all_plugins_logs_marks} =    Mark All Plugin Logs
    Start Local Dogfood SDDS3 Server

    Start Process  tail -fn0 ${BASE_LOGS_DIR}/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true
    trigger_update_now
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   check_log_contains_string_at_least_n_times    /tmp/preserve-sul-downgrade    Downgrade Log    Update success    1
    Clear Log Marks    ${all_plugins_logs_marks}
    Wait For Plugins To Be Ready    log_marks=${all_plugins_logs_marks}
    Run Keyword If  ${ExpectBaseDowngrade}    check_log_contains    Prepared ServerProtectionLinux-Base-component for downgrade    /tmp/preserve-sul-downgrade    backedup suldownloader log

    # Wait for successful update (all up to date) after downgrading
    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    200
    check_suldownloader_log_contains   Running SDDS3 update

    Mark Known Upgrade Errors
    Mark Known Downgrade Errors

    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/updatescheduler.log  updatescheduler <> Update Service (sophos-spl-update.service) failed
    # TODO LINUXDAR-2972 - expected till task is in released version
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 2] No such file or directory: '/opt/sophos-spl/tmp/policy/flags.json'
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  root <> utf8 write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    mark_expected_error_in_log  ${AV_DIR}/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry
    # When threat_detector is asked to shut down for upgrade it may have ongoing on-access scans that it has to abort
    mark_expected_error_in_log  ${AV_DIR}/log/soapd.log  OnAccessImpl <> Aborting scan, scanner is shutting down

    IF    ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
        Mark Expected Error In Log    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log    runtimedetections <> supervisor entering dormant mode due to error
        Mark Expected Error In Log    ${BASE_LOGS_DIR}/watchdog.log    ProcessMonitoringImpl <> /opt/sophos-spl/plugins/runtimedetections/bin/runtimedetections died with exit code 1
    END

    check_all_product_logs_do_not_contain_error
    check_all_product_logs_do_not_contain_critical

    Check Current Shipping Installed Correctly    ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    Run Keyword Unless
    ...  ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    ...  wait_for_log_contains_from_mark    ${rtd_mark}    Analytics started processing telemetry    ${RTD_STARTUP_TIMEOUT}
    Wait Until Keyword Succeeds
    ...    120 secs
    ...    10 secs
    ...    check_log_contains    Successfully restored old SafeStore database    ${SOPHOS_INSTALL}/logs/installation/ServerProtectionLinux-Plugin-AV_install.log    AV Install Log
    Check SafeStore Database Has Not Changed    ${safeStoreDbDirBeforeUpgrade}    ${databaseContentBeforeUpgrade}    ${safeStorePasswordBeforeUpgrade}
    Check Expected Versions Against Installed Versions    &{expectedDogfoodVersions}

    Check For downgraded logs

    # Check users haven't been removed and added back
    ${new_sspl_user_uid} =       get_uid_from_username    sophos-spl-user
    ${new_sspl_local_uid} =      get_uid_from_username    sophos-spl-local
    ${new_sspl_update_uid} =     get_uid_from_username    sophos-spl-updatescheduler
    Should Be Equal As Integers    ${sspl_user_uid}          ${new_sspl_user_uid}
    Should Be Equal As Integers    ${sspl_local_uid}         ${new_sspl_local_uid}
    Should Be Equal As Integers    ${sspl_update_uid}        ${new_sspl_update_uid}

    Stop Local SDDS3 Server
    # Upgrade back to develop to check we can upgrade from a downgraded product
    Start Local SDDS3 Server
    trigger_update_now
    Wait For Version Files to Update    &{expectedVUTVersions}

    Check SafeStore Database Has Not Changed    ${safeStoreDbDirBeforeUpgrade}    ${databaseContentBeforeUpgrade}    ${safeStorePasswordBeforeUpgrade}

    Check AV Plugin Can Scan Files
    Enable On Access Via Policy
    Check On Access Detects Threats

    # The query pack should have been re-installed
    Wait Until Created    ${Sophos_Scheduled_Query_Pack}    timeout=20s

    File Should Exist  ${AV_DIR}/log/downgrade-backup/av.log
    File Should Exist  ${AV_DIR}/log/downgrade-backup/soapd.log
    File Should Exist  ${AV_DIR}/log/downgrade-backup/sophos_threat_detector.log

    Wait Until Keyword Succeeds
    ...  180 secs
    ...  1 secs
    ...  SHS Status File Contains  ${HealthyShsStatusXmlContents}
    # Threat health returns to good after threat is cleaned up
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}

    # no cgroup check because we downgrade to aggressive mode
    Verify RTD Component Permissions
    Run Keyword Unless    ${KERNEL_VERSION_TOO_OLD_FOR_RTD}    Verify Running RTD Component Permissions
    Run Keyword Unless
    ...  ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    ...  Wait For Rtd Log Contains After Last Restart    ${RUNTIME_DETECTIONS_LOG_PATH}    Analytics started processing telemetry   timeout=${30}


We Can Upgrade From Current Shipping to VUT Without Unexpected Errors
    &{expectedReleaseVersions} =    Get Expected Versions For Recommended Tag    ${CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT}    ${CURRENT_LAUNCH_DARKLY}
    &{expectedVUTVersions} =    Get Expected Versions For Recommended Tag    ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}

    ${policy} =  Create Core Policy To enable on Access  revisionId=EnableOnAccessPolicy
    start_local_cloud_server  --initial-core-policy  ${policy}

    ${rtd_mark} =    mark_log_size    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log
    Start Local Current Shipping SDDS3 Server

    ${all_plugins_logs_marks} =    Mark All Plugin Logs
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}    sophos_log_level=DEBUG
    Wait For Plugins To Be Ready    log_marks=${all_plugins_logs_marks}      old_version=${TRUE}

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_suldownloader_log_contains_string_n_times   Update success  1
    check_suldownloader_log_contains    Running SDDS3 update

    # Update again to ensure we do not get a scheduled update later in the test run
    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120

    Check Current Shipping Installed Correctly    kernel_verion_too_old_for_rtd=${KERNEL_VERSION_TOO_OLD_FOR_RTD}  before_2024_1_group_changes=${True}
    Run Keyword Unless
    ...  ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    ...  wait_for_log_contains_from_mark    ${rtd_mark}    Analytics started processing telemetry    ${RTD_STARTUP_TIMEOUT}
    Check Expected Versions Against Installed Versions    &{expectedReleaseVersions}

    # TODO: This will fail once current shipping no longer has these. Then this check and the one below can be removed.
    File Should Exist    ${SOPHOS_INSTALL}/base/update/rootcerts/ps_rootca.crt
    File Should Exist    ${SOPHOS_INSTALL}/base/update/rootcerts/rootca.crt

    Stop Local SDDS3 Server

    # Upgrade to VUT
    ${rtd_mark} =    mark_log_size    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log
    ${watchdog_pid_before_upgrade}=     Run Process    pgrep    -f    sophos_watchdog
    Start Local SDDS3 Server

    Wait Until Keyword Succeeds
    ...  210 secs
    ...  5 secs
    ...  SHS Status File Contains  ${HealthyShsStatusXmlContents}

    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    ${all_plugins_logs_marks} =    Mark All Plugin Logs
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120
    check_suldownloader_log_contains    Running SDDS3 update
    Wait For Plugins To Be Ready    log_marks=${all_plugins_logs_marks}

    SHS Status File Contains  ${HealthyShsStatusXmlContents}
    SHS Status File Contains  ${GoodThreatHealthXmlContents}

    # Confirm that the warehouse flags supplement is installed when upgrading
    file_exists_with_permissions  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  root  sophos-spl-group  -rw-r-----

    Mark Known Upgrade Errors
    # TODO LINUXDAR-2972 - expected till task is in released version
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 2] No such file or directory: '/opt/sophos-spl/tmp/policy/flags.json'
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  root <> utf8 write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    mark_expected_error_in_log  ${AV_DIR}/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    mark_expected_error_in_log  ${AV_DIR}/log/sophos_threat_detector/sophos_threat_detector.info.log  ThreatScanner <> Failed to read customerID - using default value
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry
    # This is expected because we are restarting the avplugin to enable debug logs, we need to make sure it occurs only once though
    mark_expected_error_in_log  ${AV_DIR}/log/av.log  ScanProcessMonitor <> Exiting sophos_threat_detector with code: 15

    Run Keyword And Expect Error  *
    ...     check_log_contains_string_n_times    ${AV_DIR}/log/av.log  av.log  Exiting sophos_threat_detector with code: 15  2

    # When threat_detector is asked to shut down for upgrade it may have ongoing on-access scans that it has to abort
    mark_expected_error_in_log  ${AV_DIR}/log/soapd.log  OnAccessImpl <> Aborting scan, scanner is shutting down

    IF    ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
        Mark Expected Error In Log    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log    runtimedetections <> supervisor entering dormant mode due to error
        Mark Expected Error In Log    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log    runtimedetections <> supervisor entering dormant mode due to error
        Mark Expected Error In Log    ${BASE_LOGS_DIR}/watchdog.log    ProcessMonitoringImpl <> /opt/sophos-spl/plugins/runtimedetections/bin/runtimedetections died with exit code 1
        Mark Expected Error In Log    ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log    managementagent <> Failure on sending message to runtimedetections. Reason: No incoming data on ZMQ socket from getReply in PluginProxy
        Mark Expected Error In Log    ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log    managementagent <> Failure on sending message to runtimedetections. Reason: No incoming data on ZMQ socket from getReply in PluginProxy
    END

    check_all_product_logs_do_not_contain_error
    check_all_product_logs_do_not_contain_critical

    Check VUT Installed Correctly        ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    Run Keyword Unless
    ...  ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    ...  wait_for_log_contains_from_mark    ${rtd_mark}    Analytics started processing telemetry    ${RTD_STARTUP_TIMEOUT}
    Check Expected Versions Against Installed Versions    &{expectedVUTVersions}

    Wait Until Keyword Succeeds
    ...  180 secs
    ...  1 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}
    Check AV Plugin Can Scan Files
    Enable On Access Via Policy
    Check On Access Detects Threats

    Run Keyword Unless    ${KERNEL_VERSION_TOO_OLD_FOR_RTD}    Check RuntimeDetections Installed Correctly

    Check Update Reports Have Been Processed

    SHS Status File Contains  ${HealthyShsStatusXmlContents}
    SHS Status File Contains  ${GoodIsolationXmlContents}
    # Threat health returns to good after threat is cleaned up
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}

    ${watchdog_pid_after_upgrade}=     Run Process    pgrep    -f    sophos_watchdog
    Should Not Be Equal As Integers    ${watchdog_pid_before_upgrade.stdout}    ${watchdog_pid_after_upgrade.stdout}

    # TODO: To be removed once current shipping does not have these certs
    File Should Not Exist    ${SOPHOS_INSTALL}/base/update/rootcerts/ps_rootca.crt
    File Should Not Exist    ${SOPHOS_INSTALL}/base/update/rootcerts/rootca.crt

We Can Downgrade From VUT to Current Shipping Without Unexpected Errors
    &{expectedReleaseVersions} =    Get Expected Versions For Recommended Tag    ${CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT}    ${CURRENT_LAUNCH_DARKLY}
    &{expectedVUTVersions} =    Get Expected Versions For Recommended Tag    ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}
    ${expectBaseDowngrade} =  second_version_is_lower  ${expectedVUTVersions["baseVersion"]}  ${expectedReleaseVersions["baseVersion"]}

    start_local_cloud_server
    # Enable OnAccess
    Send Core Policy To enable on Access

    ${rtd_mark} =    mark_log_size    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log
    Start Local SDDS3 Server

    ${all_plugins_logs_marks} =    Mark All Plugin Logs
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}    sophos_log_level=DEBUG
    Wait For Plugins To Be Ready    log_marks=${all_plugins_logs_marks}

    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  1
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_suldownloader_log_contains   Update success
    check_suldownloader_log_contains    Running SDDS3 update

    # Update again to ensure we do not get a scheduled update later in the test run
    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120

    Check VUT Installed Correctly    ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    Run Keyword Unless
    ...  ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    ...  wait_for_log_contains_from_mark    ${rtd_mark}    Analytics started processing telemetry    ${RTD_STARTUP_TIMEOUT}
    Check Expected Versions Against Installed Versions    &{expectedVUTVersions}

    # TODO: To be removed once current shipping does not have these certs
    File Should Not Exist    ${SOPHOS_INSTALL}/base/update/rootcerts/ps_rootca.crt
    File Should Not Exist    ${SOPHOS_INSTALL}/base/update/rootcerts/rootca.crt

    Stop Local SDDS3 Server
    ${rtd_mark} =    mark_log_size    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log
    ${all_plugins_logs_marks} =    Mark All Plugin Logs
    Start Local Current Shipping SDDS3 Server
    Create File  ${MCS_DIR}/action/testfile
    Should Exist  ${MCS_DIR}/action/testfile
    Run Process  chown  -R  sophos-spl-local:sophos-spl-group  ${MCS_DIR}/action/testfile

    Start Process  tail -fn0 ${BASE_LOGS_DIR}/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true

    trigger_update_now
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   check_log_contains_string_at_least_n_times    /tmp/preserve-sul-downgrade    Downgrade Log    Update success    1
    Clear Log Marks    ${all_plugins_logs_marks}
    Wait For Plugins To Be Ready    log_marks=${all_plugins_logs_marks}     old_version=${TRUE}
    Run Keyword If  ${ExpectBaseDowngrade}    check_log_contains    Preparing ServerProtectionLinux-Base-component for downgrade    /tmp/preserve-sul-downgrade  backedup suldownloader log
    ${ma_mark} =  mark_log_size  ${BASE_LOGS_DIR}/sophosspl/sophos_managementagent.log

    # Wait for successful update (all up to date) after downgrading
    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark  ${ma_mark}  Action ALC_action_FakeTime.xml sent to     15
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    200

    Wait Until Keyword Succeeds
    ...   60 secs
    ...   10 secs
    ...   Should Not Exist  ${MCS_DIR}/action/testfile

    Mark Known Upgrade Errors
    Mark Known Downgrade Errors

    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/updatescheduler.log  updatescheduler <> Update Service (sophos-spl-update.service) failed
    # TODO LINUXDAR-2972 - expected till bugfix is in released version
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  root <> Atomic write failed with message: [Errno 2] No such file or directory: '/opt/sophos-spl/tmp/policy/flags.json'
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  root <> utf8 write failed with message: [Errno 13] Permission denied: '/opt/sophos-spl/tmp/policy/flags.json'
    mark_expected_error_in_log  ${AV_DIR}/log/sophos_threat_detector/sophos_threat_detector.log  ThreatScanner <> Failed to read customerID - using default value
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 2] No such file or directory: '/opt/sophos-spl/base/pluginRegistry
    # When threat_detector is asked to shut down for upgrade it may have ongoing on-access scans that it has to abort
    mark_expected_error_in_log  ${AV_DIR}/log/soapd.log  OnAccessImpl <> Aborting scan, scanner is shutting down

    IF    ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
        Mark Expected Error In Log    ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log    runtimedetections <> supervisor entering dormant mode due to error
        Mark Expected Error In Log    ${BASE_LOGS_DIR}/watchdog.log    ProcessMonitoringImpl <> /opt/sophos-spl/plugins/runtimedetections/bin/runtimedetections died with exit code 1
    END

    check_all_product_logs_do_not_contain_error
    check_all_product_logs_do_not_contain_critical

    Check Current Shipping Installed Correctly    kernel_verion_too_old_for_rtd=${KERNEL_VERSION_TOO_OLD_FOR_RTD}  before_2024_1_group_changes=${True}
    Run Keyword Unless
    ...  ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    ...  wait_for_log_contains_from_mark    ${rtd_mark}    Analytics started processing telemetry    ${RTD_STARTUP_TIMEOUT}
    Check Expected Versions Against Installed Versions    &{expectedReleaseVersions}
    Check For downgraded logs

    # TODO: This will fail once current shipping no longer has these. Then this check and the one above can be removed.
    File Should Exist    ${SOPHOS_INSTALL}/base/update/rootcerts/ps_rootca.crt
    File Should Exist    ${SOPHOS_INSTALL}/base/update/rootcerts/rootca.crt

    Stop Local SDDS3 Server
    # Upgrade back to develop to check we can upgrade from a downgraded product
    Start Local SDDS3 Server
    trigger_update_now
    Wait For Version Files to Update    &{expectedVUTVersions}

    Check AV Plugin Can Scan Files
    Enable On Access Via Policy
    Check On Access Detects Threats
    Run Keyword Unless    ${KERNEL_VERSION_TOO_OLD_FOR_RTD}    Check RuntimeDetections Installed Correctly

    # The query pack should have been re-installed
    Wait Until Created    ${Sophos_Scheduled_Query_Pack}    timeout=20s

    File Should Exist  ${AV_DIR}/log/downgrade-backup/av.log
    File Should Exist  ${AV_DIR}/log/downgrade-backup/soapd.log
    File Should Exist  ${AV_DIR}/log/downgrade-backup/sophos_threat_detector.log

    Wait Until Keyword Succeeds
    ...  180 secs
    ...  1 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  SHS Status File Contains  ${HealthyShsStatusXmlContents}

    # TODO: To be removed once current shipping does not have these certs
    File Should Not Exist    ${SOPHOS_INSTALL}/base/update/rootcerts/ps_rootca.crt
    File Should Not Exist    ${SOPHOS_INSTALL}/base/update/rootcerts/rootca.crt

SDDS3 updating respects ALC feature codes
    start_local_cloud_server
    Start Local SDDS3 Server
    ${all_plugins_logs_marks} =    Mark All Plugin Logs
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}    sophos_log_level=DEBUG
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_suldownloader_log_contains_string_n_times   Update success  1
    Wait For Plugins To Be Ready    log_marks=${all_plugins_logs_marks}

    ${sul_mark} =  mark_log_size  ${SULDownloaderLog}
    send_policy_file  alc  ${SUPPORT_FILES}/CentralXml/ALC_policy/ALC_policy_direct_just_base.xml  wait_for_policy=${True}

    wait_for_log_contains_from_mark    ${sul_mark}    Update success    80
    # Core plugins should be installed
    Directory Should Exist   ${EVENTJOURNALER_DIR}
    # Other plugins should be uninstalled
    Directory Should Not Exist   ${AV_DIR}
    Directory Should Not Exist   ${DEVICEISOLATION_DIR}
    Directory Should Not Exist   ${EDR_DIR}
    Directory Should Not Exist   ${LIVERESPONSE_DIR}
    Directory Should Not Exist   ${RTD_DIR}
    check_log_does_not_contain_after_mark    ${SULDownloaderLog}    Failed to remove path. Reason: Failed to delete file: ${SDDS3Primary}    ${sul_mark}

    send_policy_file  alc  ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml  wait_for_policy=${True}
    Wait Until Keyword Succeeds
    ...   30 secs
    ...   5 secs
    ...   File Should Contain    ${updateConfig}    AV
    ${sul_mark} =  mark_log_size  ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark  ${sul_mark}  Update success      120
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check VUT Installed Correctly        ${KERNEL_VERSION_TOO_OLD_FOR_RTD}
    Directory Should Exist   ${AV_DIR}

    Directory Should Exist   ${EDR_DIR}
    Directory Should Exist   ${LIVERESPONSE_DIR}
    Directory Should Exist   ${RTD_DIR}

SDDS3 updating with changed unused feature codes do not change version
    start_local_cloud_server
    Start Local SDDS3 Server
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}
    Override LogConf File as Global Level    DEBUG

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_suldownloader_log_contains   Update success

    &{installedVersionsBeforeUpdate} =    Get Current Installed Versions

    ${sul_mark} =  mark_log_size  ${SULDownloaderLog}
    send_policy_file  alc  ${SUPPORT_FILES}/CentralXml/ALC_policy/ALC_fake_feature_codes_policy.xml  wait_for_policy=${True}
    wait_for_log_contains_from_mark  ${sul_mark}  Update success      120

    &{installedVersionsAfterUpdate} =    Get Current Installed Versions
    Dictionaries Should Be Equal    ${installedVersionsBeforeUpdate}    ${installedVersionsAfterUpdate}

Consecutive SDDS3 Updates Without Changes Should Not Trigger Additional Installations of Components
    start_local_cloud_server
    Start Local SDDS3 Server
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_suldownloader_log_contains    Update success
    check_suldownloader_log_contains     Running SDDS3 update

    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    150
    check_log_does_not_contain_after_mark    ${SULDownloaderLog}    Installing product    ${sul_mark}

    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Base-component' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-responseactions' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-EDR' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-AV' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-liveresponse' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-RuntimeDetections' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-EventJournaler' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-DeviceIsolation' is up to date.

    ${latest_report_result} =  Run Shell Process  ls ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report* -rt | cut -f 1 | tail -n1     OnError=failed to get last report file
    all_products_in_update_report_are_up_to_date  ${latest_report_result.stdout.strip()}
    check_log_does_not_contain    extract_to  ${BASE_LOGS_DIR}/suldownloader_sync.log  sync

Sul Downloader clears cache when out of sync
    ${mark} =  mark_log_size  ${SUL_DOWNLOADER_LOG}
    start_local_cloud_server
    Start Local SDDS3 Server
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}

    wait_for_log_contains_from_mark  ${mark}  Update success    timeout=${120}
    ${mark} =  mark_log_size  ${SUL_DOWNLOADER_LOG}
    Replace Group in package config    name="0"
    Replace Group in package config    name="GranularF"
    Trigger Update Now
    wait_for_log_contains_from_mark  ${mark}  The sophos update cache was out of sync with config so it is being cleared
    wait_for_log_contains_from_mark  ${mark}  Update success   timeout=${120}

SPL Can Be Installed To A Custom Location
    [Tags]    CUSTOM_INSTALL_PATH
    [Teardown]    Upgrade Resources SDDS3 Test Teardown    ${CUSTOM_INSTALL_DIRECTORY}
    Set Local Variable    ${SOPHOS_INSTALL}    ${CUSTOM_INSTALL_DIRECTORY}

    start_local_cloud_server
    Start Local SDDS3 Server

    ${all_plugins_logs_marks} =   Mark All Plugin Logs  ${CUSTOM_INSTALL_DIRECTORY}
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080
    ...    thininstaller_source=${THIN_INSTALLER_DIRECTORY}
    ...    args=--install-dir=${CUSTOM_INSTALL_DIRECTORY_ARG}
    ...    sophos_log_level=DEBUG

    Wait For Plugins To Be Ready    log_marks=${all_plugins_logs_marks}    install_path=${CUSTOM_INSTALL_DIRECTORY}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_log_contains   Update success    ${CUSTOM_INSTALL_DIRECTORY}/logs/base/suldownloader.log    suldownloader_log

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  15 secs
    ...  SHS Status File Contains    ${HealthyShsStatusXmlContents}    ${CUSTOM_INSTALL_DIRECTORY}/base/mcs/status/SHS_status.xml
    SHS Status File Contains    ${GoodThreatHealthXmlContents}    ${CUSTOM_INSTALL_DIRECTORY}/base/mcs/status/SHS_status.xml

    # Confirm that the warehouse flags supplement is installed
    file_exists_with_permissions    ${CUSTOM_INSTALL_DIRECTORY}/base/etc/sophosspl/flags-warehouse.json  root  sophos-spl-group  -rw-r-----
    check_watchdog_service_file_has_correct_kill_mode

    check_custom_install_is_set_in_plugin_registry_files    ${CUSTOM_INSTALL_DIRECTORY}

    # Basic Installion Checks
    Should Exist    ${CUSTOM_INSTALL_DIRECTORY}
    check_correct_mcs_password_and_id_for_local_cloud_saved    ${CUSTOM_INSTALL_DIRECTORY}/base/etc/sophosspl/mcs.config
    ${result}=  Run Process  stat  -c  "%A"  /etc
    ${ExpectedPerms}=  Set Variable  "drwxr-xr-x"
    Should Be Equal As Strings  ${result.stdout}  ${ExpectedPerms}

    Check Watchdog Running
    Check Management Agent Running
    Check Update Scheduler Running
    Check Telemetry Scheduler Is Running
    Check MCS Router Running    ${CUSTOM_INSTALL_DIRECTORY}

    # Checks for AV Plugin
    File Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/av/bin/avscanner
    ${result} =    Run Process  pgrep  -f  ${CUSTOM_INSTALL_DIRECTORY}/plugins/av/sbin/av
    Should Be Equal As Integers    ${result.rc}    0    msg="stdout:${result.stdout}\nerr: ${result.stderr}"
    Check AV Plugin Can Scan Files    avscanner_path=${CUSTOM_INSTALL_DIRECTORY}/plugins/av/bin/avscanner
    Enable On Access Via Policy  ${CUSTOM_INSTALL_DIRECTORY}
    Check On Access Detects Threats
    SHS Status File Contains  ${HealthyShsStatusXmlContents}    ${CUSTOM_INSTALL_DIRECTORY}/base/mcs/status/SHS_status.xml
    # Threat health returns to good after threat is cleaned up
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}    ${CUSTOM_INSTALL_DIRECTORY}/base/mcs/status/SHS_status.xml

    # Checks for Event Journaler
    Check Event Journaler Executable Running

    # TODO LINUXDAR-8489 add checks for RTD, EDR and LiveResponse

    Directory Should Not Exist    /opt/sophos-spl
    check_all_product_logs_do_not_contain_error
    check_all_product_logs_do_not_contain_critical

Installing New Plugins Respects Custom Installation Location
    [Tags]    CUSTOM_INSTALL_PATH
    [Teardown]    Upgrade Resources SDDS3 Test Teardown    ${CUSTOM_INSTALL_DIRECTORY}
    Set Local Variable    ${SOPHOS_INSTALL}    ${CUSTOM_INSTALL_DIRECTORY}

    start_local_cloud_server    --initial-alc-policy    ${SUPPORT_FILES}/CentralXml/ALC_policy/ALC_policy_no_av.xml
    Start Local SDDS3 Server
    ${all_plugins_logs_marks} =  Mark All Plugin Logs  ${CUSTOM_INSTALL_DIRECTORY}
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080
    ...    thininstaller_source=${THIN_INSTALLER_DIRECTORY}
    ...    args=--install-dir=${CUSTOM_INSTALL_DIRECTORY_ARG}
    ...    sophos_log_level=DEBUG

    # Expect only CORE plugins to be installed
    Wait For Base To Be Ready    log_marks=${all_plugins_logs_marks}    install_path=${CUSTOM_INSTALL_DIRECTORY}
    Wait For Ej To Be Ready    log_marks=${all_plugins_logs_marks}    install_path=${CUSTOM_INSTALL_DIRECTORY}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_log_contains   Update success    ${CUSTOM_INSTALL_DIRECTORY}/logs/base/suldownloader.log    suldownloader_log

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  15 secs
    ...  SHS Status File Contains    ${HealthyShsStatusXmlContents}    ${CUSTOM_INSTALL_DIRECTORY}/base/mcs/status/SHS_status.xml
    SHS Status File Contains    ${GoodThreatHealthXmlContents}    ${CUSTOM_INSTALL_DIRECTORY}/base/mcs/status/SHS_status.xml

    #core plugins should be installed
    Directory Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/eventjournaler
    Directory Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/deviceisolation
    Directory Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/edr
    Directory Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/liveresponse
    Directory Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/runtimedetections
    #av plugin should be uninstalled
    Directory Should Not Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/av


    send_policy_file  alc  ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml  wait_for_policy=${True}    install_dir=${CUSTOM_INSTALL_DIRECTORY}
    Wait Until Keyword Succeeds
    ...   30 secs
    ...   5 secs
    ...   File Should Contain    ${CUSTOM_INSTALL_DIRECTORY}/base/update/var/updatescheduler/update_config.json    AV
    ${sul_mark} =  mark_log_size  ${CUSTOM_INSTALL_DIRECTORY}/logs/base/suldownloader.log
    trigger_update_now

    # Expect all plugins to be installed
    wait_for_log_contains_from_mark  ${sul_mark}  Update success      120
    Wait For Plugins To Be Ready    log_marks=${all_plugins_logs_marks}    install_path=${CUSTOM_INSTALL_DIRECTORY}
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Directory Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/av

    # Checks for AV Plugin
    File Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/av/bin/avscanner
    ${result} =    Run Process  pgrep  -f  ${CUSTOM_INSTALL_DIRECTORY}/plugins/av/sbin/av
    Should Be Equal As Integers    ${result.rc}    0    msg="stdout:${result.stdout}\nerr: ${result.stderr}"
    Check AV Plugin Can Scan Files    avscanner_path=${CUSTOM_INSTALL_DIRECTORY}/plugins/av/bin/avscanner
    Enable On Access Via Policy  ${CUSTOM_INSTALL_DIRECTORY}
    Check On Access Detects Threats
    SHS Status File Contains  ${HealthyShsStatusXmlContents}    ${CUSTOM_INSTALL_DIRECTORY}/base/mcs/status/SHS_status.xml
    # Threat health returns to good after threat is cleaned up
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}    ${CUSTOM_INSTALL_DIRECTORY}/base/mcs/status/SHS_status.xml

    # TODO LINUXDAR-8489 add checks for RTD, EDR and LiveResponse

*** Keywords ***
Replace Group in package config
    [Arguments]  ${pattern}
    ${content} =  Get File  ${SOPHOS_INSTALL}/base/update/var/package_config.xml
    ${output} =   Replace String   ${content}       ${pattern}      name="Fake"
    Create File     ${SOPHOS_INSTALL}/base/update/var/package_config.xml    ${output}

Install And Upgrade Test Setup
    Require Uninstalled
    Exclude RTD fallback error messages  ${SOPHOS_INSTALL}
    Register Cleanup    Check All Product Logs Do Not Contain Error
