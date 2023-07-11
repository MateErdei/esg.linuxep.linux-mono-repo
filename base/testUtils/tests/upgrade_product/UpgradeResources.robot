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
Resource    ../mdr_plugin/MDRResources.robot


*** Variables ***
${warehousedir}                                 ./tmp
${InstalledBaseVersionFile}                     ${SOPHOS_INSTALL}/base/VERSION.ini
${InstalledMDRPluginVersionFile}                ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini
${InstalledAVPluginVersionFile}                 ${SOPHOS_INSTALL}/plugins/av/VERSION.ini
${InstalledMDRSuiteVersionFile}                 ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/VERSION.ini
${InstalledEDRPluginVersionFile}                ${SOPHOS_INSTALL}/plugins/edr/VERSION.ini
${InstalledRTDPluginVersionFile}                ${SOPHOS_INSTALL}/plugins/runtimedetections/VERSION.ini
${InstalledLRPluginVersionFile}                 ${SOPHOS_INSTALL}/plugins/liveresponse/VERSION.ini
${InstalledEJPluginVersionFile}                 ${SOPHOS_INSTALL}/plugins/eventjournaler/VERSION.ini
${InstalledHBTPluginVersionFile}                ${SOPHOS_INSTALL}/plugins/heartbeat/VERSION.ini
${WarehouseBaseVersionFileExtension}            ./TestInstallFiles/ServerProtectionLinux-Base/files/base/VERSION.ini
${WarehouseMDRPluginVersionFileExtension}       ./TestInstallFiles/ServerProtectionLinux-MDR-Control/files/plugins/mtr/VERSION.ini
${WarehouseMDRSuiteVersionFileExtension}        ./TestInstallFiles/ServerProtectionLinux-MDR-DBOS-Component/files/plugins/mtr/dbos/data/VERSION.ini
${sdds3_server_output}                          /tmp/sdds3_server.log

*** Keywords ***
Upgrade Resources Test Setup
    Require Uninstalled
    Set Environment Variable  CORRUPTINSTALL  no

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
    Empty Directory  ${warehousedir}

Send ALC Policy And Prepare For Upgrade
    [Arguments]  ${PolicyPath}
    Prepare Installation For Upgrade Using Policy  ${PolicyPath}
    Send Policy File  alc  ${PolicyPath}  wait_for_policy=${True}

Prepare Installation For Upgrade Using Policy
    [Arguments]  ${PolicyPath}
    Install Upgrade Certs For Policy  ${PolicyPath}

Install Local SSL Server Cert To System
    Copy File   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem    ${SUPPORT_FILES}/https/ca/root-ca.crt
    Install System Ca Cert  ${SUPPORT_FILES}/https/ca/root-ca.crt

Wait For Initial Update To Fail
    #Only to be used when expecting the first update to fail after using the thininstaller
    Wait Until Keyword Succeeds
    ...  120 secs
    ...  5 secs
    ...  Check SulDownloader Log Contains  suldownloaderdata <> Update failed
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Remove File  ${SOPHOS_INSTALL}/logs/base/updatescheduler.log
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log

Check MDR and Base Components Inside The ALC Status
    ${statusContent} =  Get File  /opt/sophos-spl/base/mcs/status/ALC_status.xml
    Should Contain  ${statusContent}  ServerProtectionLinux-Plugin-MDR
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

Create Local SDDS3 Override
    [Arguments]  ${URLS}=https://localhost:8080  ${CDN_URL}=https://localhost:8080  ${USE_SDDS3_OVERRIDE}=true
    ${override_file_contents} =  Catenate    SEPARATOR=\n
    # these settings will instruct SulDownloader to update using SDDS3 via a local test HTTP server.
    ...  URLS = ${URLS}
    ...  CDN_URL = ${CDN_URL}
    ...  USE_SDDS3 = ${USE_SDDS3_OVERRIDE}
    Create File    ${SDDS3_OVERRIDE_FILE}    content=${override_file_contents}

Start Local SDDS3 Server
    [Arguments]    ${launchdarklyPath}=${SYSTEMPRODUCT_TEST_INPUT}/sdds3/launchdarkly    ${sdds3repoPath}=${SYSTEMPRODUCT_TEST_INPUT}/sdds3/repo
    ${handle}=  Start Process
    ...  bash  -x
    ...  ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh
    ...  python3  ${LIBS_DIRECTORY}/SDDS3server.py
    ...  --launchdarkly  ${launchdarklyPath}
    ...  --sdds3  ${sdds3repoPath}
    ...  stdout=${sdds3_server_output}
    ...  stderr=STDOUT
    Set Suite Variable    $GL_handle    ${handle}

    [Return]  ${handle}

Start Local Dogfood SDDS3 Server
    ${handle}=    Start Local SDDS3 Server    ${SYSTEMPRODUCT_TEST_INPUT}/sdds3-dogfood/launchdarkly    ${SYSTEMPRODUCT_TEST_INPUT}/sdds3-dogfood/repo
    [Return]  ${handle}

Start Local Release SDDS3 Server
    ${handle}=    Start Local SDDS3 Server    ${SYSTEMPRODUCT_TEST_INPUT}/sdds3-current_shipping/launchdarkly    ${SYSTEMPRODUCT_TEST_INPUT}/sdds3-current_shipping/repo
    [Return]  ${handle}

Start Local SDDS3 Server With Empty Repo
    Create Directory  /tmp/FakeFlags
    Create Directory    /tmp/FakeRepo
    ${handle}=    Start Local SDDS3 Server    /tmp/FakeFlags    /tmp/FakeRepo
    [Return]  ${handle}

Stop Local SDDS3 Server
    return from keyword if  "${GL_handle}" == "${EMPTY}"
    Terminate Process  ${GL_handle}  True
    Set Suite Variable    $GL_handle    ${EMPTY}
    Dump Teardown Log    ${sdds3_server_output}
    Terminate All Processes  True

Check EAP Release With AV Installed Correctly
    Check MCS Router Running
    Check MDR Plugin Installed
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check MDR Log Contains  Run SophosMTR
    Check AV Plugin Installed
    Check Installed Correctly

Check Current Release With AV Installed Correctly
    Check MCS Router Running
    Check MDR Plugin Installed
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

Check for Management Agent Failing To Send Message To MTR And Check Recovery
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log  managementagent <> Failure on sending message to mtr. Reason: No incoming data
    ${EvaluationBool} =  Does Management Agent Log Contain    managementagent <> Failure on sending message to mtr. Reason: No incoming data
    Run Keyword If
    ...  ${EvaluationBool} == ${True}
    ...  Check For MTR Recovery

Check Mtr Reconnects To Management Agent After Upgrade
    Check MDR Log Contains In Order
    ...  mtr <> Plugin Finished.
    ...  mtr <> Entering the main loop
    ...  Received new policy
    ...  RevID of the new policy

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
    ...  Version Number In Ini File Should Be    ${InstalledMDRPluginVersionFile}    ${expectedVersions["mtrVersion"]}
    
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
    ${MTRReleaseVersion} =      Get Version Number From Ini File        ${InstalledMDRPluginVersionFile}
    ${RTDReleaseVersion} =      Get RTD Version Number From Ini File    ${InstalledRTDPluginVersionFile}

    &{versions} =    Create Dictionary
    ...    baseVersion=${BaseReleaseVersion}
    ...    avVersion=${AVReleaseVersion}
    ...    edrVersion=${EDRReleaseVersion}
    ...    ejVersion=${EJReleaseVersion}
    ...    lrVersion=${LRReleaseVersion}
    ...    mtrVersion=${MTRReleaseVersion}
    ...    rtdVersion=${RTDReleaseVersion}
    [Return]    &{versions}

Get Expected Versions
    [Arguments]    ${releaseType}
    ${ExpectedBaseReleaseVersion} =     Get Version For Rigidname In SDDS3 Warehouse    ${releaseType}    ServerProtectionLinux-Base-component
    ${ExpectedAVReleaseVersion} =       Get Version For Rigidname In SDDS3 Warehouse    ${releaseType}    ServerProtectionLinux-Plugin-AV
    ${ExpectedEDRReleaseVersion} =      Get Version For Rigidname In SDDS3 Warehouse    ${releaseType}    ServerProtectionLinux-Plugin-EDR
    ${ExpectedEJReleaseVersion} =       Get Version For Rigidname In SDDS3 Warehouse    ${releaseType}    ServerProtectionLinux-Plugin-EventJournaler
    ${ExpectedLRReleaseVersion} =       Get Version For Rigidname In SDDS3 Warehouse    ${releaseType}    ServerProtectionLinux-Plugin-liveresponse
    ${ExpectedMTRReleaseVersion} =      Get Version For Rigidname In SDDS3 Warehouse    ${releaseType}    ServerProtectionLinux-Plugin-MDR
    ${ExpectedRTDReleaseVersion} =      Get Version For Rigidname In SDDS3 Warehouse    ${releaseType}    ServerProtectionLinux-Plugin-RuntimeDetections
    &{versions} =    Create Dictionary
    ...    baseVersion=${ExpectedBaseReleaseVersion}
    ...    avVersion=${ExpectedAVReleaseVersion}
    ...    edrVersion=${ExpectedEDRReleaseVersion}
    ...    ejVersion=${ExpectedEJReleaseVersion}
    ...    lrVersion=${ExpectedLRReleaseVersion}
    ...    mtrVersion=${ExpectedMTRReleaseVersion}
    ...    rtdVersion=${ExpectedRTDReleaseVersion}
    [Return]    &{versions}

Check Installed Plugins Are VUT Versions
    ${contents} =  Get File  ${EDR_DIR}/VERSION.ini
    ${edr_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   vut    ServerProtectionLinux-Plugin-EDR
    Should Contain   ${contents}   PRODUCT_VERSION = ${edr_vut_version}
    ${contents} =  Get File  ${MTR_DIR}/VERSION.ini
    ${mtr_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   vut   ServerProtectionLinux-Plugin-MDR
    Should Contain   ${contents}   PRODUCT_VERSION = ${mtr_vut_version}
    ${contents} =  Get File  ${LIVERESPONSE_DIR}/VERSION.ini
    ${liveresponse_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   vut   ServerProtectionLinux-Plugin-liveresponse
    Should Contain   ${contents}   PRODUCT_VERSION = ${liveresponse_vut_version}
    ${contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    ${eventjournaler_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   vut   ServerProtectionLinux-Plugin-EventJournaler
    Should Contain   ${contents}   PRODUCT_VERSION = ${eventjournaler_vut_version}
    ${contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    ${runtimedetections_vut_version} =  get_version_for_rigidname_in_sdds3_warehouse   vut   ServerProtectionLinux-Plugin-RuntimeDetections
    Should Contain   ${contents}   PRODUCT_VERSION = ${runtimedetections_vut_version}