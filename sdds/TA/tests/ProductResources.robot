*** Settings ***
Library    Collections
Library    OperatingSystem
Library    Process

Library    ${LIB_FILES}/FullInstallerUtils.py
Library    ${LIB_FILES}/LogUtils.py
Library    ${LIB_FILES}/MCSRouter.py
Library    ${LIB_FILES}/WarehouseUtils.py

Resource    BaseProcessesResources.robot
Resource    PluginResources.robot


*** Variables ***
${InstalledBaseVersionFile}                     ${SOPHOS_INSTALL}/base/VERSION.ini
${InstalledAVPluginVersionFile}                 ${AV_DIR}/VERSION.ini
${InstalledEDRPluginVersionFile}                ${EDR_DIR}/VERSION.ini
${InstalledEJPluginVersionFile}                 ${EVENTJOURNALER_DIR}/VERSION.ini
${InstalledLRPluginVersionFile}                 ${LIVERESPONSE_DIR}/VERSION.ini
${InstalledRAPluginVersionFile}                 ${RESPONSE_ACTIONS_DIR}/VERSION.ini
${InstalledRTDPluginVersionFile}                ${RTD_DIR}/VERSION.ini

*** Keywords ***
Get Current Installed Versions
    ${BaseReleaseVersion} =     get_version_number_from_ini_file        ${InstalledBaseVersionFile}
    ${AVReleaseVersion} =       get_version_number_from_ini_file        ${InstalledAVPluginVersionFile}
    ${EDRReleaseVersion} =      get_version_number_from_ini_file        ${InstalledEDRPluginVersionFile}
    ${EJReleaseVersion} =       get_version_number_from_ini_file        ${InstalledEJPluginVersionFile}
    ${LRReleaseVersion} =       get_version_number_from_ini_file        ${InstalledLRPluginVersionFile}
    ${RAReleaseVersion} =       get_version_number_from_ini_file        ${InstalledRAPluginVersionFile}
    ${RTDReleaseVersion} =      get_rtd_version_number_from_ini_file    ${InstalledRTDPluginVersionFile}

    &{versions} =    Create Dictionary
    ...    baseVersion=${BaseReleaseVersion}
    ...    avVersion=${AVReleaseVersion}
    ...    edrVersion=${EDRReleaseVersion}
    ...    ejVersion=${EJReleaseVersion}
    ...    lrVersion=${LRReleaseVersion}
    ...    raVersion=${RAReleaseVersion}
    ...    rtdVersion=${RTDReleaseVersion}
    [Return]    &{versions}

Get Expected Versions
    [Arguments]    ${warehouseRepoRoot}
    ${ExpectedBaseReleaseVersion} =     get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ServerProtectionLinux-Base-component
    ${ExpectedAVReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ServerProtectionLinux-Plugin-AV
    ${ExpectedEDRReleaseVersion} =      get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ServerProtectionLinux-Plugin-EDR
    ${ExpectedEJReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ServerProtectionLinux-Plugin-EventJournaler
    ${ExpectedLRReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ServerProtectionLinux-Plugin-liveresponse
    ${ExpectedRAReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ServerProtectionLinux-Plugin-responseactions
    ${ExpectedRTDReleaseVersion} =      get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ServerProtectionLinux-Plugin-RuntimeDetections
    &{versions} =    Create Dictionary
    ...    baseVersion=${ExpectedBaseReleaseVersion}
    ...    avVersion=${ExpectedAVReleaseVersion}
    ...    edrVersion=${ExpectedEDRReleaseVersion}
    ...    ejVersion=${ExpectedEJReleaseVersion}
    ...    lrVersion=${ExpectedLRReleaseVersion}
    ...    raVersion=${ExpectedRAReleaseVersion}
    ...    rtdVersion=${ExpectedRTDReleaseVersion}
    [Return]    &{versions}

Check Expected Versions Against Installed Versions
    [Arguments]    &{expectedVersions}
    &{installedVersions} =    Get Current Installed Versions
    Dictionaries Should Be Equal    ${expectedVersions}    ${installedVersions}

Wait For Version Files to Update
    [Arguments]    &{expectedVersions}
    Wait Until Keyword Succeeds
    ...  150 secs
    ...  10 secs
    ...  version_number_in_ini_file_should_be    ${InstalledBaseVersionFile}    ${expectedVersions["baseVersion"]}
    
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be    ${InstalledAVPluginVersionFile}    ${expectedVersions["avVersion"]}
    
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be    ${InstalledEDRPluginVersionFile}    ${expectedVersions["edrVersion"]}
    
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be    ${InstalledEJPluginVersionFile}    ${expectedVersions["ejVersion"]}
    
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be    ${InstalledLRPluginVersionFile}    ${expectedVersions["lrVersion"]}
    
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be    ${InstalledRAPluginVersionFile}    ${expectedVersions["raVersion"]}
    
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  5 secs
    ...  version_number_in_ini_file_should_be    ${InstalledRTDPluginVersionFile}    ${expectedVersions["rtdVersion"]}

Check Installed Correctly
    Should Exist    ${SOPHOS_INSTALL}
    check_correct_mcs_password_and_id_for_local_cloud_saved

    ${result}=  Run Process  stat  -c  "%A"  /opt
    ${ExpectedPerms}=  Set Variable  "drwxr-xr-x"
    Should Be Equal As Strings  ${result.stdout}  ${ExpectedPerms}
    ${version_number} =  get_version_number_from_ini_file    ${InstalledBaseVersionFile}
    Check Expected Base Processes Except SDU Are Running

Check EAP Release Installed Correctly
    Check Installed Correctly

    Check AV Plugin Installed
    Check Event Journaler Executable Running

Check Current Release Installed Correctly
    Check Installed Correctly
    Check Comms Component Is Not Present

    Check AV Plugin Installed
    Check Event Journaler Executable Running
    Check RuntimeDetections Installed Correctly
    Check MDR Is Not Installed

Check MDR Is Not Installed
    Directory Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr
    ${result} =    Run Process    pgrep    mtr
    Should Not Be Equal As Integers    ${result.rc}    0
    ${result} =    Run Process    pgrep    -f    dbos/SophosMTR
    Should Not Be Equal As Integers    ${result.rc}    0

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

Override LogConf File as Global Level
    [Arguments]  ${logLevel}  ${key}=VERBOSITY
    ${loggerConfPath} =  get_logger_conf_path
    Create File  ${loggerConfPath}  [global]\n${key} = ${logLevel}\n

Display All SSPL Files Installed
    [Arguments]    ${installDir}=${SOPHOS_INSTALL}
    ${handle}=  Start Process  find ${installDir}/base -not -type d | grep -v python | grep -v primarywarehouse | grep -v primary | grep -v temp_warehouse | grep -v TestInstallFiles | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  find ${installDir}/logs -not -type d | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  find ${installDir}/bin -not -type d | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  find ${installDir}/var -not -type d | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  find ${installDir}/etc -not -type d | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}

Display All SSPL Plugins Files Installed
    [Arguments]    ${installDir}=${SOPHOS_INSTALL}
    ${handle}=  Start Process  find ${installDir}/plugins/av -not -type d | grep -v lenses | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${installDir}/plugins/edr -not -type d | grep -v lenses | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${installDir}/plugins/liveresponse -not -type d | grep -v lenses | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${installDir}/plugins/eventjournaler | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${installDir}/plugins/rtd | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${installDir}/plugins/responseactions | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}