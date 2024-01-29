*** Settings ***
Library    Collections
Library    OperatingSystem
Library    Process

Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/MCSRouter.py
Library    ${COMMON_TEST_LIBS}/WarehouseUtils.py
Library    ${COMMON_TEST_LIBS}/IniUtils.py

Resource    PluginResources.robot
Resource    ${COMMON_TEST_ROBOT}/EventJournalerResources.robot


*** Variables ***
${InstalledBaseVersionFile}                     ${SOPHOS_INSTALL}/base/VERSION.ini
${InstalledAVPluginVersionFile}                 ${AV_DIR}/VERSION.ini
${InstalledDIPluginVersionFile}                 ${DEVICEISOLATION_DIR}/VERSION.ini
${InstalledEDRPluginVersionFile}                ${EDR_DIR}/VERSION.ini
${InstalledEJPluginVersionFile}                 ${EVENTJOURNALER_DIR}/VERSION.ini
${InstalledLRPluginVersionFile}                 ${LIVERESPONSE_DIR}/VERSION.ini
${InstalledRAPluginVersionFile}                 ${RESPONSE_ACTIONS_DIR}/VERSION.ini
${InstalledRTDPluginVersionFile}                ${RTD_DIR}/VERSION.ini

*** Keywords ***
Get Current Installed Versions
    # TODO: LINUXDAR-8348: Uncomment device isolation version checks following release
    ${BaseReleaseVersion} =     get_version_number_from_ini_file        ${InstalledBaseVersionFile}
    ${AVReleaseVersion} =       get_version_number_from_ini_file        ${InstalledAVPluginVersionFile}
    #${DIReleaseVersion} =       get_version_number_from_ini_file        ${InstalledDIPluginVersionFile}
    ${EDRReleaseVersion} =      get_version_number_from_ini_file        ${InstalledEDRPluginVersionFile}
    ${EJReleaseVersion} =       get_version_number_from_ini_file        ${InstalledEJPluginVersionFile}
    ${LRReleaseVersion} =       get_version_number_from_ini_file        ${InstalledLRPluginVersionFile}
    ${RAReleaseVersion} =       get_version_number_from_ini_file        ${InstalledRAPluginVersionFile}
    ${RTDReleaseVersion} =      get_rtd_version_number_from_ini_file    ${InstalledRTDPluginVersionFile}

    &{versions} =    Create Dictionary
    ...    baseVersion=${BaseReleaseVersion}
    ...    avVersion=${AVReleaseVersion}
    #...    diVersion=${DIReleaseVersion}
    ...    edrVersion=${EDRReleaseVersion}
    ...    ejVersion=${EJReleaseVersion}
    ...    lrVersion=${LRReleaseVersion}
    ...    raVersion=${RAReleaseVersion}
    ...    rtdVersion=${RTDReleaseVersion}
    [Return]    &{versions}

Get Expected Versions For Recommended Tag
    [Arguments]    ${warehouseRepoRoot}    ${launchdarkly_root}
    ${ExpectedBaseReleaseVersion} =     get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Base-component
    ${ExpectedAVReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-AV
    #${ExpectedDIReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-DeviceIsolation
    ${ExpectedEDRReleaseVersion} =      get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-EDR
    ${ExpectedEJReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-EventJournaler
    ${ExpectedLRReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-liveresponse
    ${ExpectedRAReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-responseactions
    ${ExpectedRTDReleaseVersion} =      get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-RuntimeDetections
    &{versions} =    Create Dictionary
    ...    baseVersion=${ExpectedBaseReleaseVersion}
    ...    avVersion=${ExpectedAVReleaseVersion}
    #...    diVersion=${ExpectedDIReleaseVersion}
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

#    Wait Until Keyword Succeeds
#        ...  200 secs
#        ...  5 secs
#        ...  version_number_in_ini_file_should_be    ${InstalledDIPluginVersionFile}    ${expectedVersions["diVersion"]}
    
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

# Actually for Current Shipping or Dogfood
Check Current Shipping Installed Correctly
    [Arguments]    ${kernel_verion_too_old_for_rtd}=${False}    ${before_2024_1_group_changes}=${False}
    Check Installed Correctly
    Check Comms Component Is Not Present
    Check MCS Router Running

    Check AV Plugin Installed
    Check SafeStore Installed Correctly    before_2024_1_group_changes=${before_2024_1_group_changes}
    Check Event Journaler Executable Running
    Run Keyword Unless    ${kernel_verion_too_old_for_rtd}    Check RuntimeDetections Installed Correctly
    Check MDR Is Not Installed

Check VUT Installed Correctly
    [Arguments]    ${kernel_verion_too_old_for_rtd}=${False}
    Check Installed Correctly
    Check Comms Component Is Not Present
    Check MCS Router Running

    Check AV Plugin Installed
    Check SafeStore Installed Correctly    before_2024_1_group_changes=${False}
    Check Event Journaler Executable Running
    Run Keyword Unless    ${kernel_verion_too_old_for_rtd}    Check RuntimeDetections Installed Correctly
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
