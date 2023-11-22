*** Settings ***
Library    Collections
Library    OperatingSystem

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
Resource    ${COMMON_TEST_ROBOT}/EventJournalerResources.robot
Resource    ${COMMON_TEST_ROBOT}/GeneralUtilsResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Suite Setup      Upgrade Resources Suite Setup

Test Setup       require_uninstalled
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

# Thin installer appends sophos-spl to the argument
${CUSTOM_INSTALL_DIRECTORY}    /home/parent/sophos-spl
${CUSTOM_INSTALL_DIRECTORY_ARG}    /home/parent
${RPATHCheckerLog}                          /tmp/rpath_checker.log

*** Test Cases ***
Sul Downloader fails update if expected product missing from SUS
    start_local_cloud_server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_FakePlugin.xml
    Start Local SDDS3 Server

    configure_and_run_SDDS3_thininstaller    ${18}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}
    check_thininstaller_log_contains    Failed to connect to repository: Product doesn't match any suite: ServerProtectionLinux-Plugin-Fake

We Can Upgrade From Dogfood to VUT Without Unexpected Errors
    # TODO once 2023.43/2023.4 is in dogfood: remove ARM64 exclusion
    [Tags]    EXCLUDE_ARM64
    &{expectedDogfoodVersions} =    Get Expected Versions For Recommended Tag    ${DOGFOOD_WAREHOUSE_REPO_ROOT}    ${DOGFOOD_LAUNCH_DARKLY}
    # TODO LINUXDAR-8265: Remove is_using_version_workaround
    &{expectedVUTVersions} =    Get Expected Versions For Recommended Tag    ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}    is_using_version_workaround=${True}

    start_local_cloud_server
    # Enable OnAccess
    send_policy_file    core    ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

    Start Local Dogfood SDDS3 Server
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}
    Override LogConf File as Global Level    DEBUG

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

    Check EAP Release Installed Correctly
    ${safeStoreDbDirBeforeUpgrade} =    List Files In Directory    ${SAFESTORE_DB_DIR}
    ${safeStorePasswordBeforeUpgrade} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}
    ${databaseContentBeforeUpgrade} =    get_contents_of_safestore_database
    Check Expected Versions Against Installed Versions    &{expectedDogfoodVersions}
    Stop Local SDDS3 Server

    # Upgrading to VUT
    ${watchdog_pid_before_upgrade}=     Run Process    pgrep    -f    sophos_watchdog
    Start Local SDDS3 Server

    Wait Until Threat Detector Running

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  15 secs
    ...  SHS Status File Contains    ${HealthyShsStatusXmlContents}

    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120

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

    Run Keyword And Expect Error  *
    ...     check_log_contains_string_n_times  ${AV_DIR}/log/av.log  av.log  Exiting sophos_threat_detector with code: 15  2

    check_all_product_logs_do_not_contain_error
    check_all_product_logs_do_not_contain_critical

    Check Current Release Installed Correctly
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
    # TODO once 2023.43/2023.4 is in dogfood: remove ARM64 exclusion
    [Tags]    EXCLUDE_ARM64
    &{expectedDogfoodVersions} =    Get Expected Versions For Recommended Tag    ${DOGFOOD_WAREHOUSE_REPO_ROOT}    ${DOGFOOD_LAUNCH_DARKLY}
    # TODO LINUXDAR-8265: Remove is_using_version_workaround
    &{expectedVUTVersions} =    Get Expected Versions For Recommended Tag    ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}    is_using_version_workaround=${True}
    ${expectBaseDowngrade} =  second_version_is_lower  ${expectedVUTVersions["baseVersion"]}  ${expectedDogfoodVersions["baseVersion"]}

    start_local_cloud_server
    # Enable OnAccess
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

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

    Check Current Release Installed Correctly
    ${safeStoreDbDirBeforeUpgrade} =    List Files In Directory    ${SAFESTORE_DB_DIR}
    ${safeStorePasswordBeforeUpgrade} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}
    ${databaseContentBeforeUpgrade} =    get_contents_of_safestore_database
    Check Expected Versions Against Installed Versions    &{expectedVUTVersions}


    ${sspl_user_uid} =       get_uid_from_username    sophos-spl-user
    ${sspl_local_uid} =      get_uid_from_username    sophos-spl-local
    ${sspl_update_uid} =     get_uid_from_username    sophos-spl-updatescheduler

    Stop Local SDDS3 Server
    # Changing the policy here will result in an automatic update
    # Note when downgrading from a release with live response to a release without live response
    # results in a second update.
    Start Local Dogfood SDDS3 Server

    Start Process  tail -fn0 ${BASE_LOGS_DIR}/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true
    trigger_update_now
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   check_log_contains_string_at_least_n_times    /tmp/preserve-sul-downgrade    Downgrade Log    Update success    1
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

    check_all_product_logs_do_not_contain_error
    check_all_product_logs_do_not_contain_critical

    Check EAP Release Installed Correctly
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

We Can Upgrade From Current Shipping to VUT Without Unexpected Errors
    # TODO once 2023.4 is released: remove ARM64 exclusion
    [Tags]    EXCLUDE_ARM64
    &{expectedReleaseVersions} =    Get Expected Versions For Recommended Tag    ${CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT}    ${CURRENT_LAUNCH_DARKLY}
    # TODO LINUXDAR-8265: Remove is_using_version_workaround
    &{expectedVUTVersions} =    Get Expected Versions For Recommended Tag    ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}    is_using_version_workaround=${True}

    start_local_cloud_server
    # Enable OnAccess
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

    Start Local Current Shipping SDDS3 Server
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}
    Override LogConf File as Global Level    DEBUG

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

    Check EAP Release Installed Correctly
    Check Expected Versions Against Installed Versions    &{expectedReleaseVersions}

    Stop Local SDDS3 Server

    # Upgrade to VUT
    ${watchdog_pid_before_upgrade}=     Run Process    pgrep    -f    sophos_watchdog
    Start Local SDDS3 Server

    Wait Until Keyword Succeeds
    ...  210 secs
    ...  5 secs
    ...  SHS Status File Contains  ${HealthyShsStatusXmlContents}

    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120
    check_suldownloader_log_contains    Running SDDS3 update

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
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry
    # This is expected because we are restarting the avplugin to enable debug logs, we need to make sure it occurs only once though
    mark_expected_error_in_log  ${AV_DIR}/log/av.log  ScanProcessMonitor <> Exiting sophos_threat_detector with code: 15

    Run Keyword And Expect Error  *
    ...     check_log_contains_string_n_times    ${AV_DIR}/log/av.log  av.log  Exiting sophos_threat_detector with code: 15  2

    # When threat_detector is asked to shut down for upgrade it may have ongoing on-access scans that it has to abort
    mark_expected_error_in_log  ${AV_DIR}/log/soapd.log  OnAccessImpl <> Aborting scan, scanner is shutting down

    check_all_product_logs_do_not_contain_error
    check_all_product_logs_do_not_contain_critical

    Check Current Release Installed Correctly
    Check Expected Versions Against Installed Versions    &{expectedVUTVersions}

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

    ${watchdog_pid_after_upgrade}=     Run Process    pgrep    -f    sophos_watchdog
    Should Not Be Equal As Integers    ${watchdog_pid_before_upgrade.stdout}    ${watchdog_pid_after_upgrade.stdout}

We Can Downgrade From VUT to Current Shipping Without Unexpected Errors
    # TODO once 2023.4 is released: remove ARM64 exclusion
    [Tags]    EXCLUDE_ARM64
    &{expectedReleaseVersions} =    Get Expected Versions For Recommended Tag    ${CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT}    ${CURRENT_LAUNCH_DARKLY}
    # TODO LINUXDAR-8265: Remove is_using_version_workaround
    &{expectedVUTVersions} =    Get Expected Versions For Recommended Tag    ${VUT_WAREHOUSE_ROOT}    ${VUT_LAUNCH_DARKLY}    is_using_version_workaround=${True}
    ${expectBaseDowngrade} =  second_version_is_lower  ${expectedVUTVersions["baseVersion"]}  ${expectedReleaseVersions["baseVersion"]}

    start_local_cloud_server
    # Enable OnAccess
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

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

    Check Current Release Installed Correctly
    Check Expected Versions Against Installed Versions    &{expectedVUTVersions}

    Stop Local SDDS3 Server
    # Changing the policy here will result in an automatic update
    # Note when downgrading from a release with live response to a release without live response
    # results in a second update.
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

    check_all_product_logs_do_not_contain_error
    check_all_product_logs_do_not_contain_critical

    Check EAP Release Installed Correctly
    Check Expected Versions Against Installed Versions    &{expectedReleaseVersions}
    Check For downgraded logs

    Stop Local SDDS3 Server
    # Upgrade back to develop to check we can upgrade from a downgraded product
    Start Local SDDS3 Server
    trigger_update_now
    Wait For Version Files to Update    &{expectedVUTVersions}

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
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  5 secs
    ...  SHS Status File Contains  ${HealthyShsStatusXmlContents}

SDDS3 updating respects ALC feature codes
    [Tags]    RTD_CHECKED
    start_local_cloud_server
    Start Local SDDS3 Server
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_suldownloader_log_contains_string_n_times   Update success  1

    ${sul_mark} =  mark_log_size  ${SULDownloaderLog}
    send_policy_file  alc  ${SUPPORT_FILES}/CentralXml/ALC_CORE_only_feature_code.policy.xml  wait_for_policy=${True}

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
    ...   Check Current Release Installed Correctly
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
    send_policy_file  alc  ${SUPPORT_FILES}/CentralXml/ALC_fake_feature_codes_policy.xml  wait_for_policy=${True}
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

SPL Can Be Installed To A Custom Location
    [Tags]    CUSTOM_INSTALL_PATH
    [Teardown]    Upgrade Resources SDDS3 Test Teardown    ${CUSTOM_INSTALL_DIRECTORY}
    Set Local Variable    ${SOPHOS_INSTALL}    ${CUSTOM_INSTALL_DIRECTORY}

    # TODO: LINUXDAR-7773 use ALC policy containing all feature codes once RTD supports custom installs
    start_local_cloud_server    --initial-alc-policy    ${SUPPORT_FILES}/CentralXml/ALC_BaseWithAVPolicy.xml
    Start Local SDDS3 Server
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080
    ...    thininstaller_source=${THIN_INSTALLER_DIRECTORY}
    ...    args=--install-dir=${CUSTOM_INSTALL_DIRECTORY_ARG}

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
    Enable On Access Via Policy
    Check On Access Detects Threats
    SHS Status File Contains  ${HealthyShsStatusXmlContents}    ${CUSTOM_INSTALL_DIRECTORY}/base/mcs/status/SHS_status.xml
    # Threat health returns to good after threat is cleaned up
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}    ${CUSTOM_INSTALL_DIRECTORY}/base/mcs/status/SHS_status.xml

    # Checks for Event Journaler
    Check Event Journaler Executable Running

    # TODO LINUXDAR-7773 add checks for RTD, EDR and LiveResponse

    Directory Should Not Exist    /opt/sophos-spl
    check_all_product_logs_do_not_contain_error
    check_all_product_logs_do_not_contain_critical

Installing New Plugins Respects Custom Installation Location
    [Tags]    CUSTOM_INSTALL_PATH
    [Teardown]    Upgrade Resources SDDS3 Test Teardown    ${CUSTOM_INSTALL_DIRECTORY}
    Set Local Variable    ${SOPHOS_INSTALL}    ${CUSTOM_INSTALL_DIRECTORY}

    start_local_cloud_server    --initial-alc-policy    ${SUPPORT_FILES}/CentralXml/ALC_CORE_only_feature_code.policy.xml
    Start Local SDDS3 Server
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080
    ...    thininstaller_source=${THIN_INSTALLER_DIRECTORY}
    ...    args=--install-dir=${CUSTOM_INSTALL_DIRECTORY_ARG}

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
    #other plugins should be uninstalled
    Directory Should Not Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/av
    Directory Should Not Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/deviceisolation
    Directory Should Not Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/edr
    Directory Should Not Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/liveresponse
    Directory Should Not Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/rtd

    # TODO: LINUXDAR-7773 use ALC policy containing all feature codes once RTD supports custom installs
    send_policy_file  alc  ${SUPPORT_FILES}/CentralXml/ALC_BaseWithAVPolicy.xml  wait_for_policy=${True}    install_dir=${CUSTOM_INSTALL_DIRECTORY}
    Wait Until Keyword Succeeds
    ...   30 secs
    ...   5 secs
    ...   File Should Contain    ${CUSTOM_INSTALL_DIRECTORY}/base/update/var/updatescheduler/update_config.json    AV
    ${sul_mark} =  mark_log_size  ${CUSTOM_INSTALL_DIRECTORY}/logs/base/suldownloader.log
    trigger_update_now
    wait_for_log_contains_from_mark  ${sul_mark}  Update success      120
    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Directory Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/av

    # Checks for AV Plugin
    File Should Exist   ${CUSTOM_INSTALL_DIRECTORY}/plugins/av/bin/avscanner
    ${result} =    Run Process  pgrep  -f  ${CUSTOM_INSTALL_DIRECTORY}/plugins/av/sbin/av
    Should Be Equal As Integers    ${result.rc}    0    msg="stdout:${result.stdout}\nerr: ${result.stderr}"
    Check AV Plugin Can Scan Files    avscanner_path=${CUSTOM_INSTALL_DIRECTORY}/plugins/av/bin/avscanner
    Enable On Access Via Policy
    Check On Access Detects Threats
    SHS Status File Contains  ${HealthyShsStatusXmlContents}    ${CUSTOM_INSTALL_DIRECTORY}/base/mcs/status/SHS_status.xml
    # Threat health returns to good after threat is cleaned up
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  SHS Status File Contains  ${GoodThreatHealthXmlContents}    ${CUSTOM_INSTALL_DIRECTORY}/base/mcs/status/SHS_status.xml

    # TODO LINUXDAR-7773 add checks for RTD, EDR and LiveResponse

