*** Settings ***
Library    Collections
Library    OperatingSystem
Library    Process

Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/MCSRouter.py
Library    ${COMMON_TEST_LIBS}/WarehouseUtils.py

Resource    PluginResources.robot
Resource    ${COMMON_TEST_ROBOT}/EventJournalerResources.robot


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

# TODO LINUXDAR-8265: Remove is_using_version_workaround
Get Expected Versions For Recommended Tag
    [Arguments]    ${warehouseRepoRoot}    ${launchdarkly_root}    ${is_using_version_workaround}=${False}
    ${ExpectedBaseReleaseVersion} =     get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Base-component    is_using_version_workaround=${is_using_version_workaround}
    ${ExpectedAVReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-AV    is_using_version_workaround=${is_using_version_workaround}
    ${ExpectedEDRReleaseVersion} =      get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-EDR    is_using_version_workaround=${is_using_version_workaround}
    ${ExpectedEJReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-EventJournaler    is_using_version_workaround=${is_using_version_workaround}
    ${ExpectedLRReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-liveresponse    is_using_version_workaround=${is_using_version_workaround}
    ${ExpectedRAReleaseVersion} =       get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-responseactions    is_using_version_workaround=${is_using_version_workaround}
    ${ExpectedRTDReleaseVersion} =      get_version_for_rigidname_in_sdds3_warehouse    ${warehouseRepoRoot}    ${launchdarkly_root}    ServerProtectionLinux-Plugin-RuntimeDetections    is_using_version_workaround=${is_using_version_workaround}
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
