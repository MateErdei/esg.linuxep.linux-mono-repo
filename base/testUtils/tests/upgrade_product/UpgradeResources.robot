*** Settings ***
Library    Collections
Library    OperatingSystem
Library    Process
Library    String

Library    ${LIBS_DIRECTORY}/CentralUtils.py
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/MCSRouter.py
Library    ${LIBS_DIRECTORY}/OSUtils.py
Library    ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py
Library    ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library    ${LIBS_DIRECTORY}/UpdateServer.py
Library    ${LIBS_DIRECTORY}/WarehouseUtils.py

Resource    ../av_plugin/AVResources.robot
Resource    ../av_plugin/SafeStoreResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../mcs_router/McsRouterResources.robot


*** Variables ***
${InstalledBaseVersionFile}                     ${SOPHOS_INSTALL}/base/VERSION.ini
${InstalledAVPluginVersionFile}                 ${SOPHOS_INSTALL}/plugins/av/VERSION.ini
${InstalledEDRPluginVersionFile}                ${SOPHOS_INSTALL}/plugins/edr/VERSION.ini
${InstalledRTDPluginVersionFile}                ${SOPHOS_INSTALL}/plugins/runtimedetections/VERSION.ini
${InstalledLRPluginVersionFile}                 ${SOPHOS_INSTALL}/plugins/liveresponse/VERSION.ini
${InstalledEJPluginVersionFile}                 ${SOPHOS_INSTALL}/plugins/eventjournaler/VERSION.ini
${InstalledHBTPluginVersionFile}                ${SOPHOS_INSTALL}/plugins/heartbeat/VERSION.ini
${sdds3_server_output}                          /tmp/sdds3_server.log

*** Keywords ***
Upgrade Resources SDDS3 Test Teardown
    Stop Local SDDS3 Server
    Upgrade Resources Test Teardown

Upgrade Resources Test Teardown
    [Arguments]  ${UninstallAudit}=True
    Run Keyword If Test Failed    Dump Cloud Server Log
    Run Keyword If Test Failed    Log XDR Intermediary File
    General Test Teardown
    Run Keyword If Test Failed    Dump Thininstaller Log
    Run Keyword If Test Failed    Log Status Of Sophos Spl
    Run Keyword If Test Failed    Dump Teardown Log    ${SOPHOS_INSTALL}/base/update/var/config.json
    Run Keyword If Test Failed    Dump Teardown Log    /tmp/preserve-sul-downgrade
    Remove File  /tmp/preserve-sul-downgrade
    Stop Local Cloud Server
    Cleanup Local Warehouse And Thininstaller
    Require Uninstalled
    Cleanup Temporary Folders

Upgrade Resources Suite Setup
    Set Suite Variable    ${GL_handle}       ${EMPTY}
    Set Suite Variable    ${GL_UC_handle}    ${EMPTY}
    Generate Local Ssl Certs If They Dont Exist
    Install Local SSL Server Cert To System
    Regenerate Certificates
    Set Local CA Environment Variable

Upgrade Resources Suite Teardown
    Run Process    make    clean    cwd=${SUPPORT_FILES}/https/

Cleanup Local Warehouse And Thininstaller
    Stop Proxy Servers
    Stop Update Server
    Cleanup Files

Install Local SSL Server Cert To System
    Copy File   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem    ${SUPPORT_FILES}/https/ca/root-ca.crt
    Install System Ca Cert  ${SUPPORT_FILES}/https/ca/root-ca.crt

Check AV and Base Components Inside The ALC Status
    ${statusContent} =  Get File  /opt/sophos-spl/base/mcs/status/ALC_status.xml
    Should Contain  ${statusContent}  ServerProtectionLinux-Plugin-AV
    Should Contain  ${statusContent}  ServerProtectionLinux-Base

Check Status Has Changed
     [Arguments]  ${status1}
     ${status2} =  Get File  /opt/sophos-spl/base/mcs/status/ALC_status.xml
     Should Not Be Equal As Strings  ${status1}  ${status2}


Log XDR Intermediary File
    Run Keyword And Ignore Error   Log File  ${SOPHOS_INSTALL}/plugins/edr/var/xdr_intermediary


Mark Known Upgrade Errors
    # TODO: LINUXDAR-7318 - expected till bugfix is in released version
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  /opt/sophos-spl/base/bin/UpdateScheduler died with signal 9

    #LINUXDAR-4015 There won't be a fix for this error, please check the ticket for more info
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/plugins/runtimedetections/log/runtimedetections.log  runtimedetections <> Could not enter supervised child process

Mark Known Downgrade Errors
    # Deliberatly missing the last part of these lines so it will work on all plugin registry files.
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> Failed to load plugin file: /opt/sophos-spl/base/pluginRegistry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.utils.plugin_registry <> [Errno 13] Permission denied: '/opt/sophos-spl/base/pluginRegistry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/wdctl.log  wdctlActions <> Plugin "responseactions" not in registry
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/wdctl.log  wdctlActions <> Plugin "liveresponse" not in registry

Create Local SDDS3 Override
    [Arguments]  ${URLS}=https://localhost:8080  ${CDN_URL}=https://localhost:8080
    ${override_file_contents} =  Catenate    SEPARATOR=\n
    # these settings will instruct SulDownloader to update using SDDS3 via a local test HTTP server.
    ...  URLS = ${URLS}
    ...  CDN_URL = ${CDN_URL}
    Create File    ${SDDS3_OVERRIDE_FILE}    content=${override_file_contents}

Start Local SDDS3 Server
    [Arguments]    ${launchdarklyPath}=${VUT_WAREHOUSE_ROOT}/launchdarkly    ${sdds3repoPath}=${VUT_WAREHOUSE_ROOT}/repo
    ${handle}=  Start Process
    ...  bash  -x
    ...  ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh
    ...  python3  ${LIBS_DIRECTORY}/SDDS3server.py
    ...  --launchdarkly  ${launchdarklyPath}
    ...  --sdds3  ${sdds3repoPath}
    ...  stdout=${sdds3_server_output}
    ...  stderr=STDOUT
    Set Suite Variable    $GL_handle    ${handle}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Can Curl Url    https://localhost:8080
    [Return]  ${handle}

Start Local Dogfood SDDS3 Server
    ${handle}=    Start Local SDDS3 Server    ${DOGFOOD_WAREHOUSE_ROOT}/launchdarkly    ${DOGFOOD_WAREHOUSE_ROOT}/repo
    [Return]  ${handle}

Start Local Current Shipping SDDS3 Server
    ${handle}=    Start Local SDDS3 Server    ${CURRENT_SHIPPING_WAREHOUSE_ROOT}/launchdarkly    ${CURRENT_SHIPPING_WAREHOUSE_ROOT}/repo
    [Return]  ${handle}

Start Local SDDS3 Server With Empty Repo
    Create Directory  /tmp/FakeFlags
    Create Directory    /tmp/FakeRepo
    ${handle}=    Start Local SDDS3 Server    /tmp/FakeFlags    /tmp/FakeRepo
    [Return]  ${handle}

Debug Local SDDS3 Server
    ${result} =  Run Process  pstree  -a  stderr=STDOUT
    Log  pstree: ${result.rc} : ${result.stdout}
    ${result} =  Run Process  netstat  --pl  --inet  stderr=STDOUT
    Log  netstat: ${result.rc} : ${result.stdout}
    ${result} =  Run Process  ss  -plt  stderr=STDOUT
    Log  ss: ${result.rc} : ${result.stdout}

Stop Local SDDS3 Server
    return from keyword if  "${GL_handle}" == "${EMPTY}"
    Run Keyword and Ignore Error  Run Keyword If Test Failed  Debug Local SDDS3 Server
    ${result} =  Terminate Process  ${GL_handle}  True
    Set Suite Variable    $GL_handle    ${EMPTY}
    Dump Teardown Log    ${sdds3_server_output}
    Log  SDDS3_server rc = ${result.rc}
    Terminate All Processes  True

Check EAP Release With AV Installed Correctly
    Check MCS Router Running
    Check AV Plugin Installed
    Check Installed Correctly

Check Current Release With AV Installed Correctly
    Check MCS Router Running
    Check AV Plugin Installed
    Check SafeStore Installed Correctly
    Check Installed Correctly
    Check Comms Component Is Not Present

Check Comms Component Is Not Present
    Verify Group Removed    sophos-spl-network
    Verify User Removed    sophos-spl-network

    File Should Not Exist    ${SOPHOS_INSTALL}/base/bin/CommsComponent
    File Should Not Exist    ${SOPHOS_INSTALL}/logs/base/sophosspl/comms_component.log
    File Should Not Exist    ${SOPHOS_INSTALL}/base/pluginRegistry/commscomponent.json

    Directory Should Not Exist   ${SOPHOS_INSTALL}/var/sophos-spl-comms
    Directory Should Not Exist   ${SOPHOS_INSTALL}/var/comms
    Directory Should Not Exist   ${SOPHOS_INSTALL}/logs/base/sophos-spl-comms

Check Installed Correctly
    Should Exist    ${SOPHOS_INSTALL}
    Check Correct MCS Password And ID For Local Cloud Saved

    ${result}=  Run Process  stat  -c  "%A"  /opt
    ${ExpectedPerms}=  Set Variable  "drwxr-xr-x"
    Should Be Equal As Strings  ${result.stdout}  ${ExpectedPerms}
    ${version_number} =  Get Version Number From Ini File  ${InstalledBaseVersionFile}
    Check Expected Base Processes Except SDU Are Running

Check N Update Reports Processed
    [Arguments]  ${updateReportCount}
    ${processedFileCount} =    Count Files In Directory    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports
    Should Be Equal As Integers    ${processedFileCount}    ${updateReportCount}

Check Update Reports Have Been Processed
    Directory Should Exist    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports
    ${filesInProcessedDir} =    List Files In Directory    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports
    Log    ${filesInProcessedDir}
    ${filesInUpdateVar} =    List Files In Directory    ${SOPHOS_INSTALL}/base/update/var/updatescheduler
    Log    ${filesInUpdateVar}
    ${UpdateReportCount} =    Count Files In Directory    ${SOPHOS_INSTALL}/base/update/var/updatescheduler    update_report[0-9]*.json

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Check N Update Reports Processed    ${UpdateReportCount}

    Should Contain    ${filesInProcessedDir}[0]    update_report
    Should Not Contain    ${filesInProcessedDir}[0]    update_report.json
    Should Contain    ${filesInUpdateVar}    ${filesInProcessedDir}[0]


Check Expected Versions Against Installed Versions
    [Arguments]    &{expectedVersions}
    &{installedVersions} =    Get Current Installed Versions
    Dictionaries Should Be Equal    ${expectedVersions}    ${installedVersions}

Wait For Version Files to Update
    [Arguments]    &{expectedVersions}
    Wait Until Keyword Succeeds
    ...  150 secs
    ...  10 secs
    ...  Version Number In Ini File Should Be    ${InstalledBaseVersionFile}    ${expectedVersions["baseVersion"]}
    
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Version Number In Ini File Should Be    ${InstalledAVPluginVersionFile}    ${expectedVersions["avVersion"]}
    
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Version Number In Ini File Should Be    ${InstalledEDRPluginVersionFile}    ${expectedVersions["edrVersion"]}
    
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Version Number In Ini File Should Be    ${InstalledEJPluginVersionFile}    ${expectedVersions["ejVersion"]}
    
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Version Number In Ini File Should Be    ${InstalledLRPluginVersionFile}    ${expectedVersions["lrVersion"]}

    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  Version Number In Ini File Should Be    ${InstalledRTDPluginVersionFile}    ${expectedVersions["rtdVersion"]}

Get Current Installed Versions
    ${BaseReleaseVersion} =     Get Version Number From Ini File        ${InstalledBaseVersionFile}
    ${AVReleaseVersion} =       Get Version Number From Ini File        ${InstalledAVPluginVersionFile}
    ${EDRReleaseVersion} =      Get Version Number From Ini File        ${InstalledEDRPluginVersionFile}
    ${EJReleaseVersion} =       Get Version Number From Ini File        ${InstalledEJPluginVersionFile}
    ${LRReleaseVersion} =       Get Version Number From Ini File        ${InstalledLRPluginVersionFile}
    ${RTDReleaseVersion} =      Get RTD Version Number From Ini File    ${InstalledRTDPluginVersionFile}

    &{versions} =    Create Dictionary
    ...    baseVersion=${BaseReleaseVersion}
    ...    avVersion=${AVReleaseVersion}
    ...    edrVersion=${EDRReleaseVersion}
    ...    ejVersion=${EJReleaseVersion}
    ...    lrVersion=${LRReleaseVersion}
    ...    rtdVersion=${RTDReleaseVersion}
    [Return]    &{versions}

Get Expected Versions
    [Arguments]    ${warehouseRoot}
    ${ExpectedBaseReleaseVersion} =     Get Version For Rigidname In SDDS3 Warehouse    ${warehouseRoot}/repo    ServerProtectionLinux-Base-component
    ${ExpectedAVReleaseVersion} =       Get Version For Rigidname In SDDS3 Warehouse    ${warehouseRoot}/repo    ServerProtectionLinux-Plugin-AV
    ${ExpectedEDRReleaseVersion} =      Get Version For Rigidname In SDDS3 Warehouse    ${warehouseRoot}/repo    ServerProtectionLinux-Plugin-EDR
    ${ExpectedEJReleaseVersion} =       Get Version For Rigidname In SDDS3 Warehouse    ${warehouseRoot}/repo    ServerProtectionLinux-Plugin-EventJournaler
    ${ExpectedLRReleaseVersion} =       Get Version For Rigidname In SDDS3 Warehouse    ${warehouseRoot}/repo    ServerProtectionLinux-Plugin-liveresponse
    ${ExpectedRTDReleaseVersion} =      Get Version For Rigidname In SDDS3 Warehouse    ${warehouseRoot}/repo    ServerProtectionLinux-Plugin-RuntimeDetections
    &{versions} =    Create Dictionary
    ...    baseVersion=${ExpectedBaseReleaseVersion}
    ...    avVersion=${ExpectedAVReleaseVersion}
    ...    edrVersion=${ExpectedEDRReleaseVersion}
    ...    ejVersion=${ExpectedEJReleaseVersion}
    ...    lrVersion=${ExpectedLRReleaseVersion}
    ...    rtdVersion=${ExpectedRTDReleaseVersion}
    [Return]    &{versions}

Check Installed Plugins Are VUT Versions
    ${contents} =  Get File  ${EDR_DIR}/VERSION.ini
    ${edr_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   ${VUT_WAREHOUSE_ROOT}/repo    ServerProtectionLinux-Plugin-EDR
    Should Contain   ${contents}   PRODUCT_VERSION = ${edr_vut_version}
    ${contents} =  Get File  ${LIVERESPONSE_DIR}/VERSION.ini
    ${liveresponse_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   ${VUT_WAREHOUSE_ROOT}/repo    ServerProtectionLinux-Plugin-liveresponse
    Should Contain   ${contents}   PRODUCT_VERSION = ${liveresponse_vut_version}
    ${contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    ${eventjournaler_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   ${VUT_WAREHOUSE_ROOT}/repo    ServerProtectionLinux-Plugin-EventJournaler
    Should Contain   ${contents}   PRODUCT_VERSION = ${eventjournaler_vut_version}
    ${contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    ${runtimedetections_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   ${VUT_WAREHOUSE_ROOT}/repo    ServerProtectionLinux-Plugin-RuntimeDetections
    Should Contain   ${contents}   PRODUCT_VERSION = ${runtimedetections_vut_version}