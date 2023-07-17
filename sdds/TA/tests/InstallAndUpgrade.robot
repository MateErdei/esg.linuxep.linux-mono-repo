*** Settings ***
Library    Collections
Library    OperatingSystem

Library    ${LIB_FILES}/FullInstallerUtils.py
Library    ${LIB_FILES}/LogUtils.py
Library    ${LIB_FILES}/MCSRouter.py
Library    ${LIB_FILES}/OSUtils.py
Library    ${LIB_FILES}/SafeStoreUtils.py
Library    ${LIB_FILES}/ThinInstallerUtils.py
Library    ${LIB_FILES}/UpdateSchedulerHelper.py
Library    ${LIB_FILES}/WarehouseUtils.py

Resource    BaseProcessesResources.robot
Resource    GeneralUtilsResources.robot
Resource    PluginResources.robot
Resource    ProductResources.robot
Resource    UpgradeResources.robot

Suite Setup      Upgrade Resources Suite Setup
Suite Teardown   Upgrade Resources Suite Teardown

Test Setup       Upgrade Resources Test Setup
Test Teardown    Upgrade Resources SDDS3 Test Teardown

Test Timeout  10 mins
Force Tags    PACKAGE


*** Variables ***
${BaseEdrAndMtrAndAVDogfoodPolicy}    ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT-1.xml
${BaseEdrAndMtrAndAVReleasePolicy}    ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_Release.xml
${BaseEdrAndMtrAndAVVUTPolicy}        ${GeneratedWarehousePolicies}/base_edr_and_mtr_and_av_VUT.xml

${SULDownloaderLog}                   ${BASE_LOGS_DIR}/suldownloader.log
${SULDownloaderLogDowngrade}          ${BASE_LOGS_DIR}/downgrade-backup/suldownloader.log
${Sophos_Scheduled_Query_Pack}        ${EDR_DIR}/etc/osquery.conf.d/sophos-scheduled-query-pack.conf
${updateConfig}                       ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json

${SDDS3Primary}                       ${SOPHOS_INSTALL}/base/update/cache/sdds3primary
${SDDS3PrimaryRepository}             ${SOPHOS_INSTALL}/base/update/cache/sdds3primaryrepository

${HealthyShsStatusXmlContents}        <item name="health" value="1" />
${GoodThreatHealthXmlContents}        <item name="threat" value="1" />


*** Test Cases ***
Sul Downloader fails update if expected product missing from SUS
    start_local_cloud_server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_FakePlugin.xml
    Start Local SDDS3 Server

    configure_and_run_SDDS3_thininstaller    ${18}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}
    check_thininstaller_log_contains    Failed to connect to repository: Product doesn't match any suite: ServerProtectionLinux-Plugin-Fake

We Can Upgrade From Dogfood to VUT Without Unexpected Errors
    &{expectedDogfoodVersions} =    Get Expected Versions    ${DOGFOOD_WAREHOUSE_REPO_ROOT}
    &{expectedVUTVersions} =    Get Expected Versions    ${VUT_WAREHOUSE_REPO_ROOT}

    start_local_cloud_server    --initial-alc-policy    ${BaseEdrAndMtrAndAVDogfoodPolicy}
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
    ...   check_suldownloader_log_contains_string_n_times    Update success    2
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
    Start Local SDDS3 Server
    Send ALC Policy And Prepare For Upgrade    ${BaseEdrAndMtrAndAVVUTPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  check_policy_written_match_file    ALC-1_policy.xml    ${BaseEdrAndMtrAndAVVUTPolicy}
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
    # If the policy comes down fast enough SophosMtr will not have started by the time MTR plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    mark_expected_error_in_log  ${PLUGINS_DIR}/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    mark_expected_error_in_log  ${PLUGINS_DIR}/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found
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

We Can Downgrade From VUT to Dogfood Without Unexpected Errors
    &{expectedDogfoodVersions} =    Get Expected Versions    ${DOGFOOD_WAREHOUSE_REPO_ROOT}
    &{expectedVUTVersions} =    Get Expected Versions    ${VUT_WAREHOUSE_REPO_ROOT}
    ${expectBaseDowngrade} =  second_version_is_lower  ${expectedVUTVersions["baseVersion"]}  ${expectedDogfoodVersions["baseVersion"]}

    start_local_cloud_server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
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

    Directory Should Not Exist   ${BASE_LOGS_DIR}/downgrade-backup

    ${sspl_user_uid} =       get_uid_from_username    sophos-spl-user
    ${sspl_local_uid} =      get_uid_from_username    sophos-spl-local
    ${sspl_update_uid} =     get_uid_from_username    sophos-spl-updatescheduler

    Stop Local SDDS3 Server
    # Changing the policy here will result in an automatic update
    # Note when downgrading from a release with live response to a release without live response
    # results in a second update.
    Start Local Dogfood SDDS3 Server
    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVDogfoodPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  check_policy_written_match_file  ALC-1_policy.xml  ${BaseEdrAndMtrAndAVDogfoodPolicy}

    Start Process  tail -fn0 ${BASE_LOGS_DIR}/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true
    trigger_update_now
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   check_log_contains_string_at_least_n_times    /tmp/preserve-sul-downgrade    Downgrade Log    Update success    1
    Run Keyword If  ${ExpectBaseDowngrade}    check_log_contains    Preparing ServerProtectionLinux-Base-component for downgrade    ${SULDownloaderLogDowngrade}    backedup suldownloader log

    # Wait for successful update (all up to date) after downgrading
    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    200
    check_suldownloader_log_contains   Running SDDS3 update

    Mark Known Upgrade Errors
    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    mark_expected_error_in_log  ${PLUGINS_DIR}/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    mark_expected_error_in_log  ${PLUGINS_DIR}/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found
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
    ...    check_log_contains    Successfully restored old SafeStore database    /tmp/preserve-sul-downgrade    Downgrade Log
    Check SafeStore Database Has Not Changed    ${safeStoreDbDirBeforeUpgrade}    ${databaseContentBeforeUpgrade}    ${safeStorePasswordBeforeUpgrade}
    Check Expected Versions Against Installed Versions    &{expectedDogfoodVersions}

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
    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVVUTPolicy}
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
    &{expectedReleaseVersions} =    Get Expected Versions    ${CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT}
    &{expectedVUTVersions} =    Get Expected Versions    ${VUT_WAREHOUSE_REPO_ROOT}

    start_local_cloud_server    --initial-alc-policy    ${BaseEdrAndMtrAndAVReleasePolicy}
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
    ...   check_suldownloader_log_contains_string_n_times   Update success  2
    check_suldownloader_log_contains    Running SDDS3 update

    # Update again to ensure we do not get a scheduled update later in the test run
    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120

    Check EAP Release Installed Correctly
    Check Expected Versions Against Installed Versions    &{expectedReleaseVersions}

    Stop Local SDDS3 Server

    # Upgrade to VUT
    Start Local SDDS3 Server
    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVVUTPolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  check_policy_written_match_file  ALC-1_policy.xml  ${BaseEdrAndMtrAndAVVUTPolicy}

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
    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    mark_expected_error_in_log  ${PLUGINS_DIR}/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    mark_expected_error_in_log  ${PLUGINS_DIR}/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found
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

We Can Downgrade From VUT to Current Shipping Without Unexpected Errors
    &{expectedReleaseVersions} =    Get Expected Versions    ${CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT}
    &{expectedVUTVersions} =    Get Expected Versions    ${VUT_WAREHOUSE_REPO_ROOT}
    ${expectBaseDowngrade} =  second_version_is_lower  ${expectedVUTVersions["baseVersion"]}  ${expectedReleaseVersions["baseVersion"]}

    start_local_cloud_server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
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

    Directory Should Not Exist   ${BASE_LOGS_DIR}/downgrade-backup

    Stop Local SDDS3 Server
    # Changing the policy here will result in an automatic update
    # Note when downgrading from a release with live response to a release without live response
    # results in a second update.
    Start Local Current Shipping SDDS3 Server
    Create File  ${MCS_DIR}/action/testfile
    Should Exist  ${MCS_DIR}/action/testfile
    Run Process  chown  -R  sophos-spl-local:sophos-spl-group  ${MCS_DIR}/action/testfile

    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVReleasePolicy}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  check_policy_written_match_file  ALC-1_policy.xml  ${BaseEdrAndMtrAndAVReleasePolicy}
    Start Process  tail -fn0 ${BASE_LOGS_DIR}/suldownloader.log > /tmp/preserve-sul-downgrade  shell=true

    trigger_update_now
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   check_log_contains_string_at_least_n_times    /tmp/preserve-sul-downgrade    Downgrade Log    Update success    1
    Run Keyword If  ${ExpectBaseDowngrade}    check_log_contains    Preparing ServerProtectionLinux-Base-component for downgrade    ${SULDownloaderLogDowngrade}  backedup suldownloader log
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
    # If the policy comes down fast enough SophosMtr will not have started by the time mtr plugin is restarted
    # This is only an issue with versions of base before we started using boost process
    mark_expected_error_in_log  ${PLUGINS_DIR}/mtr/log/mtr.log  ProcessImpl <> The PID -1 does not exist or is not a child of the calling process.
    #  This is raised when PluginAPI has been changed so that it is no longer compatible until upgrade has completed.
    mark_expected_error_in_log  ${PLUGINS_DIR}/mtr/log/mtr.log  mtr <> Policy is invalid: RevID not found
    mark_expected_error_in_log  ${BASE_LOGS_DIR}/sophosspl/updatescheduler.log  updatescheduler <> Update Service (sophos-spl-update.service) failed
    # TODO LINUXDAR-2972 - expected till bugfix is in released version
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
    Check Expected Versions Against Installed Versions    &{expectedReleaseVersions}

    Stop Local SDDS3 Server
    # Upgrade back to develop to check we can upgrade from a downgraded product
    Start Local SDDS3 Server
    Send ALC Policy And Prepare For Upgrade  ${BaseEdrAndMtrAndAVVUTPolicy}
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
    start_local_cloud_server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    Start Local SDDS3 Server
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_suldownloader_log_contains_string_n_times   Update success  2

    ${sul_mark} =  mark_log_size  ${SULDownloaderLog}
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_CORE_only_feature_code.policy.xml  wait_for_policy=${True}

    wait_for_log_contains_from_mark    ${sul_mark}    Update success    80
    # Core plugins should be installed
    Directory Should Exist   ${EVENTJOURNALER_DIR}
    Directory Should Exist   ${RTD_DIR}
    # Other plugins should be uninstalled
    Directory Should Not Exist    ${AV_DIR}
    Directory Should Not Exist   ${EDR_DIR}
    Directory Should Not Exist   ${LIVERESPONSE_DIR}
    Directory Should Not Exist   ${PLUGINS_DIR}/mtr
    check_log_does_not_contain_after_mark    ${SULDownloaderLog}    Failed to remove path. Reason: Failed to delete file: ${SDDS3Primary}    ${sul_mark}

    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml  wait_for_policy=${True}
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
    Directory Should Exist   ${PLUGINS_DIR}/mtr

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
    Send Policy File  alc  ${SUPPORT_FILES}/CentralXml/ALC_fake_feature_codes_policy.xml  wait_for_policy=${True}
    wait_for_log_contains_from_mark  ${sul_mark}  Update success      120

    &{installedVersionsAfterUpdate} =    Get Current Installed Versions
    Dictionaries Should Be Equal    ${installedVersionsBeforeUpdate}    ${installedVersionsAfterUpdate}

SDDS3 updating when warehouse files have not changed does not extract the zip files
    start_local_cloud_server
    Start Local SDDS3 Server

    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_suldownloader_log_contains   Update success

    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120
    check_suldownloader_log_contains_string_n_times   Generating the report file  2
    check_log_does_not_contain    extract_to  ${BASE_LOGS_DIR}/suldownloader_sync.log  sync

Consecutive SDDS3 Updates Without Changes Should Not Trigger Additional Installations of Components
    start_local_cloud_server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
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
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-MDR' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-EDR' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-AV' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-liveresponse' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-RuntimeDetections' is up to date.
    wait_for_log_contains_from_mark   ${sul_mark}    Downloaded Product line: 'ServerProtectionLinux-Plugin-EventJournaler' is up to date.

    ${latest_report_result} =  Run Shell Process  ls ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report* -rt | cut -f 1 | tail -n1     OnError=failed to get last report file
    all_products_in_update_report_are_up_to_date  ${latest_report_result.stdout.strip()}

Schedule Query Pack Next Exists in SDDS3 and is Equal to Schedule Query Pack
    start_local_cloud_server  --initial-alc-policy  ${BaseEdrAndMtrAndAVVUTPolicy}
    Start Local SDDS3 Server
    configure_and_run_SDDS3_thininstaller    ${0}    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}

    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   check_suldownloader_log_contains   Update success

    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    Directory Should Not Be Empty    ${SDDS3Primary}
    Wait Until Keyword Succeeds
    ...    60 secs
    ...    5 secs
    ...    Directory Should Not Be Empty    ${SDDS3PrimaryRepository}

    Wait Until Created    ${SDDS3Primary}/ServerProtectionLinux-Plugin-EDR    timeout=300s
    Wait Until Created    ${SDDS3Primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack    timeout=120s
    Wait Until Created    ${SDDS3Primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack_next    timeout=120s

    ${scheduled_query_pack} =             Get File    ${SDDS3Primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack/sophos-scheduled-query-pack.conf
    ${scheduled_query_pack_next} =        Get File    ${SDDS3Primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack_next/sophos-scheduled-query-pack.conf
    Should Be Equal As Strings            ${scheduled_query_pack}    ${scheduled_query_pack_next}

    ${scheduled_query_pack_mtr} =         Get File    ${SDDS3Primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack/sophos-scheduled-query-pack.mtr.conf
    ${scheduled_query_pack_next_mtr} =    Get File    ${SDDS3Primary}/ServerProtectionLinux-Plugin-EDR/scheduled_query_pack_next/sophos-scheduled-query-pack.mtr.conf
    Should Be Equal As Strings            ${scheduled_query_pack_mtr}    ${scheduled_query_pack_next_mtr}