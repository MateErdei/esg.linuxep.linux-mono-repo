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
Force Tags    PACKAGE


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

    @{expectedFixedVersions} =    Get Fixed Versions    e4ac6bd8-fcb3-432c-892d-a2e135756094    2c641477c410c3d2073e7d17b7df5328af30afb902bf4ef8f11e490cb8bd5e24e1f23d7796ab85a83cf2635ffc3fe24d5174    q    ${hostname}
    FOR    ${expectedFixedVersion}     IN      @{expectedFixedVersions}
        log to console    Fixed Version: ${expectedFixedVersion}
        ${result} =   Run Process     bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIB_FILES}/GatherReleaseWarehouses.py --dest ${INPUT_DIRECTORY} --fixed-version "${expectedFixedVersion}"  shell=true
        Log  ${result.stdout}
        Log  ${result.stderr}
        Should Be Equal As Strings   ${result.rc}  0
        ${result} =   Run Process     ls -l ${INPUT_DIRECTORY}  shell=true	
        Log  ${result.stdout}	
        Log  ${result.stderr}
        ${result} =   Run Process     ls -l ${INPUT_DIRECTORY}/repo  shell=true	
        Log  ${result.stdout}	
        Log  ${result.stderr}
        ${result} =   Run Process     ls -l "${INPUT_DIRECTORY}/FTS\ 2023.2.0.74"  shell=true	
        Log  ${result.stdout}	
        Log  ${result.stderr}
        Check Upgrade From Fixed Version to VUT    ${expectedFixedVersion}
    END


*** Keywords ***
Check Upgrade From Fixed Version to VUT
    [Arguments]  ${fixedVersion}
    &{expectedFixedVersions} =    Get Expected Versions    ${INPUT_DIRECTORY}/${fixedVersion}/repo
    &{expectedVUTVersions} =      Get Expected Versions    ${INPUT_DIRECTORY}/repo

    Start Local Cloud Server
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml

    ${handle}=    Start Local SDDS3 Server    ${INPUT_DIRECTORY}/${fixedVersion}/repo/launchdarkly    ${INPUT_DIRECTORY}/${fixedVersion}/repo
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
    Check Expected Versions Against Installed Versions    &{expectedFixedVersions}

    Stop Local SDDS3 Server
    ${handle}=    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
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
