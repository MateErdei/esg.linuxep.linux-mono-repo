*** Settings ***
Library    Collections
Library    OperatingSystem

Library    ${LIB_FILES}/CentralUtils.py
Library    ${LIB_FILES}/FullInstallerUtils.py
Library    ${LIB_FILES}/LogUtils.py
Library    ${LIB_FILES}/MCSRouter.py
Library    ${LIB_FILES}/OSUtils.py
Library    ${LIB_FILES}/SafeStoreUtils.py
Library    ${LIB_FILES}/TelemetryUtils.py
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

Test Setup       require_uninstalled
Test Teardown    Upgrade Resources SDDS3 Test Teardown

Test Timeout  10 mins
Force Tags    PACKAGE    FIXED_VERSIONS


*** Variables ***
${SULDownloaderLog}                         ${BASE_LOGS_DIR}/suldownloader.log
${SULDownloaderSyncLog}                     ${BASE_LOGS_DIR}/suldownloader_sync.log
${SULDownloaderLogDowngrade}                ${BASE_LOGS_DIR}/downgrade-backup/suldownloader.log
${UpdateSchedulerLog}                       ${BASE_LOGS_DIR}/sophosspl/updatescheduler.log
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
Product Can Upgrade From Fixed Versions to VUT Without Unexpected Errors
    ${build_jwt} =    Get File    ${SUPPORT_FILES}/jenkins/jwt_token.txt
    Set Environment Variable    BUILD_JWT         ${build_jwt}
    ${hostname} =    Get Hostname

    ${central_api_client_id} =  Get Environment Variable    CENTRAL_API_CLIENT_ID
    ${central_api_client_secret} =  Get Environment Variable    CENTRAL_API_CLIENT_SECRET
    @{expectedFixedVersions} =    Get Fixed Versions    ${central_api_client_id}    ${central_api_client_secret}    q    ${hostname}
    FOR    ${expectedFixedVersion}     IN      @{expectedFixedVersions}
        log to console    Fixed Version: ${expectedFixedVersion}
        ${result} =   Run Process     bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIB_FILES}/GatherReleaseWarehouses.py --dest ${INPUT_DIRECTORY} --fixed-version "${expectedFixedVersion}"  shell=true
        Log  ${result.stdout}
        Log  ${result.stderr}
        Should Be Equal As Strings   ${result.rc}  0
        Check Upgrade From Fixed Version to VUT    ${expectedFixedVersion}
    END


Product Can Downgrade From VUT to Fixed Versions Without Unexpected Errors
    ${build_jwt} =    Get File    ${SUPPORT_FILES}/jenkins/jwt_token.txt
    Set Environment Variable    BUILD_JWT         ${build_jwt}
    ${hostname} =    Get Hostname

    ${central_api_client_id} =  Get Environment Variable    CENTRAL_API_CLIENT_ID
    ${central_api_client_secret} =  Get Environment Variable    CENTRAL_API_CLIENT_SECRET
    @{expectedFixedVersions} =    Get Fixed Versions    ${central_api_client_id}    ${central_api_client_secret}    q    ${hostname}
    FOR    ${expectedFixedVersion}     IN      @{expectedFixedVersions}
        log to console    Fixed Version: ${expectedFixedVersion}
        ${result} =   Run Process     bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIB_FILES}/GatherReleaseWarehouses.py --dest ${INPUT_DIRECTORY} --fixed-version "${expectedFixedVersion}"  shell=true
        Log  ${result.stdout}
        Log  ${result.stderr}
        Should Be Equal As Strings   ${result.rc}  0
        Check Downgrade From VUT to Fixed Version    ${expectedFixedVersion}
    END


*** Keywords ***
Check Upgrade From Fixed Version to VUT
    [Arguments]  ${fixedVersion}
    &{expectedFixedVersions} =    Get Expected Versions    ${INPUT_DIRECTORY}/${fixedVersion}/repo
    &{expectedVUTVersions} =      Get Expected Versions    ${INPUT_DIRECTORY}/repo

    Start Local Cloud Server
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

    ${handle}=    Start Local SDDS3 Server    ${INPUT_DIRECTORY}/${fixedVersion}/launchdarkly    ${INPUT_DIRECTORY}/${fixedVersion}/repo
    Set Suite Variable    $GL_handle    ${handle}

    Configure And Run SDDS3 Thininstaller    0    https://localhost:8080    https://localhost:8080    thininstaller_source=${THIN_INSTALLER_DIRECTORY}
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

    Check EAP Release Installed Correctly
    ${safeStoreDbDirBeforeUpgrade} =    List Files In Directory    ${SAFESTORE_DB_DIR}
    ${safeStorePasswordBeforeUpgrade} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}
    ${databaseContentBeforeUpgrade} =    Get Contents of SafeStore Database
    Check Expected Versions Against Installed Versions    &{expectedFixedVersions}

    Stop Local SDDS3 Server
    ${handle}=    Start Local SDDS3 Server
    Set Suite Variable    $GL_handle    ${handle}
    Wait Until Threat Detector Running

    Wait Until Keyword Succeeds
    ...  120 secs
    ...  15 secs
    ...  SHS Status File Contains    ${HealthyShsStatusXmlContents}

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

    Check Current Release Installed Correctly
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

Check Downgrade From VUT to Fixed Version
    [Arguments]  ${fixedVersion}
    &{expectedFixedVersions} =    Get Expected Versions    ${INPUT_DIRECTORY}/${fixedVersion}/repo
    &{expectedVUTVersions} =      Get Expected Versions    ${INPUT_DIRECTORY}/repo
    ${expectBaseDowngrade} =  second_version_is_lower  ${expectedVUTVersions["baseVersion"]}  ${expectedFixedVersions["baseVersion"]}

    start_local_cloud_server
    # Enable OnAccess
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

    ${handle}=    Start Local SDDS3 Server    ${INPUT_DIRECTORY}/${fixedVersion}/launchdarkly    ${INPUT_DIRECTORY}/${fixedVersion}/repo
    Set Suite Variable    ${GL_handle}    ${handle}

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
    Run Keyword If  ${ExpectBaseDowngrade}    check_log_contains    Preparing ServerProtectionLinux-Base-component for downgrade    /tmp/preserve-sul-downgrade    backedup suldownloader log

    # Wait for successful update (all up to date) after downgrading
    ${sul_mark} =    mark_log_size    ${SULDownloaderLog}
    trigger_update_now
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    200
    check_suldownloader_log_contains   Running SDDS3 update

    Mark Known Upgrade Errors
    Mark Known Downgrade Errors
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
    ...    check_log_contains    Successfully restored old SafeStore database    ${SOPHOS_INSTALL}/logs/installation/ServerProtectionLinux-Plugin-AV_install.log    AV Install Log
    Check SafeStore Database Has Not Changed    ${safeStoreDbDirBeforeUpgrade}    ${databaseContentBeforeUpgrade}    ${safeStorePasswordBeforeUpgrade}
    Check Expected Versions Against Installed Versions    &{expectedFixedVersions}

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
