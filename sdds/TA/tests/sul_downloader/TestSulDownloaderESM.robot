*** Settings ***
Suite Setup      Upgrade Resources Suite Setup

Test Setup       Require Uninstalled
Test Teardown    Run Keywords
...                Remove Environment Variable  http_proxy    AND
...                Remove Environment Variable  https_proxy  AND
...                Stop Proxy If Running    AND
...                Stop Proxy Servers   AND
...                Clean up fake warehouse  AND
...                Upgrade Resources SDDS3 Test Teardown

Library     DateTime
Library     ${COMMON_TEST_LIBS}/FakeSDDS3UpdateCacheUtils.py
Library     ${COMMON_TEST_LIBS}/PolicyUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/ThinInstallerUtils.py

Resource    ${COMMON_TEST_ROBOT}/EDRResources.robot
Resource    ${COMMON_TEST_ROBOT}/GeneralUtilsResources.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/SchedulerUpdateResources.robot
Resource    ${COMMON_TEST_ROBOT}/SDDS3Resources.robot
Resource    ${COMMON_TEST_ROBOT}/SulDownloaderResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Force Tags  TAP_PARALLEL1  SULDOWNLOADER

*** Variables ***
${UpdateSchedulerLog}                       ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
${SULDownloaderLog}                         ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${tmpPolicy}                                /tmp/tmpALC.xml
${staticflagfile}                           linuxep.json

*** Test Cases ***
Valid ESM Entry Is Requested By Suldownloader
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_normal_cloud_sub    LTS 2023.1.1    f4d41a16-b751-4195-a7b2-1f109d49469d
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}
    Register Cleanup  Remove File  ${tmpPolicy}

    ${update_mark} =  mark_log_size    ${UpdateSchedulerLog}
    ${sul_mark} =  mark_log_size    ${SULDownloaderLog}

    Start Local Cloud Server    --initial-alc-policy  ${tmpPolicy}
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}

    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override

    Register With Local Cloud Server

    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}

    wait_for_log_contains_from_mark  ${update_mark}  Using FixedVersion LTS 2023.1.1 with token f4d41a16-b751-4195-a7b2-1f109d49469d    20

    File Should Contain  ${UPDATE_CONFIG}     JWToken

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   File Should Contain    ${sdds3_server_log}     fixed_version_token f4d41a16-b751-4195-a7b2-1f109d49469d
    wait_for_log_contains_from_mark  ${sul_mark}  Doing product and supplement update


We Install Flags Correctly For Static Suites
    Setup SUS static
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_scheduled_updates    ${fixed_version_name}    ${fixed_version_token}
    Remove File  ${tmpPolicy}
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}

    ${sul_mark} =  mark_log_size    ${SULDownloaderLog}

    Start Local Cloud Server    --initial-alc-policy  ${tmpPolicy}
    Start Local SDDS3 Server    launchdarklyPath=${tmpLaunchDarkly}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override

    Register With Local Cloud Server

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   File Should Contain    ${sdds3_server_log}     fixed_version_token
    wait_for_log_contains_from_mark  ${sul_mark}  Doing product and supplement update


    Wait Until Keyword Succeeds
    ...   60 secs
    ...   5 secs
    ...   File Should Contain  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json     always
    File Should Contain  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json     false


Invalid ESM Policy Entry Is Caught By Suldownloader
    #Invalid because we are only providing a name for the fixed_version
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_normal_cloud_sub    LTS 2023.1.1
    Create File    ${tmpPolicy}    ${esm_enabled_alc_policy}
    Register Cleanup  Remove File  ${tmpPolicy}

    ${update_mark} =  mark_log_size    ${UpdateSchedulerLog}

    Start Local Cloud Server    --initial-alc-policy  ${tmpPolicy}
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}

    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override

    Register With Local Cloud Server

    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}

    wait_for_log_contains_from_mark  ${update_mark}  Using version RECOMMENDED

    ${sul_mark} =  mark_log_size    ${SULDownloaderLog}

    Trigger Update Now
    wait_for_log_contains_from_mark  ${sul_mark}   ESM feature is not valid. Name: LTS 2023.1.1 and Token:


Absent ESM Field Does Not Appear In Update Config
    ${update_mark} =  mark_log_size    ${UpdateSchedulerLog}

    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}

    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override

    Register With Local Cloud Server

    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}

    wait_for_log_contains_from_mark  ${update_mark}  Using version RECOMMENDED

    File Should Not Contain  ${UPDATE_CONFIG}     fixed_version_token


New Fixed Version Token Doesnt Trigger Immediate Update When Scheduled Updates Are Enabled
    [Tags]  BASE_DOWNGRADE  THIN_INSTALLER  INSTALLER  UNINSTALLER

    Setup SUS all develop
    Remove File  ${tmpPolicy}
    ${non_esm_alc_policy_scheduled} =    populate_cloud_subscription_with_scheduled
    Create File  ${tmpPolicy}   ${non_esm_alc_policy_scheduled}
    Register Cleanup    Remove File    ${tmpPolicy}

    Start Local Cloud Server  --initial-alc-policy  ${tmpPolicy}

    Start Local SDDS3 Server    launchdarklyPath=${tmpLaunchDarkly}
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    configure_and_run_SDDS3_thininstaller  0  https://localhost:8080   https://localhost:8080
    Override LogConf File as Global Level  DEBUG
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    150

    Wait For Suldownloader To Finish

    Setup SUS static
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_scheduled_updates    ${fixed_version_name}    ${fixed_version_token}
    Remove File  ${tmpPolicy}
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}

    ${update_mark} =  mark_log_size    ${UpdateSchedulerLog}
    ${sul_mark} =  mark_log_size    ${SULDownloaderLog}

    Send Policy File  alc    ${tmpPolicy}
    wait_for_log_contains_from_mark  ${update_mark}    Using FixedVersion ${fixed_version_name} with token ${fixed_version_token}    20
    wait_for_log_contains_from_mark  ${sul_mark}    Doing supplement-only update


New Fixed Version Token Does Trigger Immediate Update With Update Now When Scheduled Updates Are Enabled
    [Tags]  BASE_DOWNGRADE  THIN_INSTALLER  INSTALLER  UNINSTALLER

    Setup SUS all develop
    Remove File  ${tmpPolicy}
    ${non_esm_alc_policy_scheduled} =    populate_cloud_subscription_with_scheduled
    Create File  ${tmpPolicy}   ${non_esm_alc_policy_scheduled}
    Register Cleanup    Remove File    ${tmpPolicy}

    Start Local Cloud Server  --initial-alc-policy  ${tmpPolicy}

    Start Local SDDS3 Server    launchdarklyPath=${tmpLaunchDarkly}
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    configure_and_run_SDDS3_thininstaller  0  https://localhost:8080   https://localhost:8080
    Override LogConf File as Global Level  DEBUG
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    150

    Wait For Suldownloader To Finish

    Setup SUS static
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_scheduled_updates    ${fixed_version_name}    ${fixed_version_token}
    Remove File  ${tmpPolicy}
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}

    ${update_mark} =  mark_log_size    ${UpdateSchedulerLog}
    ${sul_mark} =  mark_log_size    ${SULDownloaderLog}

    Send Policy File  alc    ${tmpPolicy}
    wait_for_log_contains_from_mark  ${update_mark}    Using FixedVersion ${fixed_version_name} with token ${fixed_version_token}    20
    Trigger Update Now

    wait_for_log_contains_from_mark  ${sul_mark}   "token": "${fixed_version_token}"
    wait_for_log_contains_from_mark  ${sul_mark}   "name": "${fixed_version_name}"


New Fixed Version Token Does Trigger Immediate Update When Paused Updates Are Enabled
    [Tags]  BASE_DOWNGRADE  THIN_INSTALLER  INSTALLER  UNINSTALLER

    Setup SUS all develop
    Remove File  ${tmpPolicy}
    ${non_esm_alc_policy} =    populate_cloud_subscription_with_paused
    Create File  ${tmpPolicy}   ${non_esm_alc_policy}
    Register Cleanup    Remove File    ${tmpPolicy}

    Start Local Cloud Server  --initial-alc-policy  ${tmpPolicy}

    Start Local SDDS3 Server    launchdarklyPath=${tmpLaunchDarkly}
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    configure_and_run_SDDS3_thininstaller  0  https://localhost:8080   https://localhost:8080
    Override LogConf File as Global Level  DEBUG
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    150

    Wait For Suldownloader To Finish

    Setup SUS static
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_paused_updates    ${fixed_version_name}    ${fixed_version_token}
    Remove File  ${tmpPolicy}
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}

    ${update_mark} =  mark_log_size    ${UpdateSchedulerLog}
    ${sul_mark} =  mark_log_size    ${SULDownloaderLog}

    Send Policy File  alc    ${tmpPolicy}
    wait_for_log_contains_from_mark  ${update_mark}    Using FixedVersion ${fixed_version_name} with token ${fixed_version_token}    20

    wait_for_log_contains_from_mark  ${sul_mark}   "token": "${fixed_version_token}"
    wait_for_log_contains_from_mark  ${sul_mark}   "name": "${fixed_version_name}"


New Fixed Version Token Does Trigger Immediate Update
    [Tags]  BASE_DOWNGRADE  THIN_INSTALLER  INSTALLER  UNINSTALLER

    Setup SUS all develop
    Remove File  ${tmpPolicy}
    ${non_esm_alc_policy} =    populate_cloud_subscription
    Create File  ${tmpPolicy}   ${non_esm_alc_policy}
    Register Cleanup    Remove File    ${tmpPolicy}

    Start Local Cloud Server  --initial-alc-policy  ${tmpPolicy}

    Start Local SDDS3 Server    launchdarklyPath=${tmpLaunchDarkly}
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    configure_and_run_SDDS3_thininstaller  0  https://localhost:8080   https://localhost:8080
    Override LogConf File as Global Level  DEBUG
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    150

    Wait For Suldownloader To Finish

    Setup SUS static
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_normal_cloud_sub    ${fixed_version_name}    ${fixed_version_token}
    Remove File  ${tmpPolicy}
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}

    ${update_mark} =  mark_log_size    ${UpdateSchedulerLog}
    ${sul_mark} =  mark_log_size    ${SULDownloaderLog}
    Send Policy File  alc    ${tmpPolicy}
    wait_for_log_contains_from_mark  ${update_mark}    Using FixedVersion ${fixed_version_name} with token ${fixed_version_token}    20

    wait_for_log_contains_from_mark  ${sul_mark}   "token": "${fixed_version_token}"
    wait_for_log_contains_from_mark  ${sul_mark}   "name": "${fixed_version_name}"


Install all plugins static-999 then downgrade to all plugins static
    [Tags]  BASE_DOWNGRADE  THIN_INSTALLER  INSTALLER  UNINSTALLER    RTD_CHECKED

    Setup SUS static 999
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_normal_cloud_sub    ${fixed_version_name}    ${fixed_version_token}
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}
    Register Cleanup  Remove File  ${tmpPolicy}

    Start Local Cloud Server  --initial-alc-policy  ${tmpPolicy}

    Start Local SDDS3 Server    launchdarklyPath=${tmpLaunchDarkly}
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    configure_and_run_SDDS3_thininstaller  0  https://localhost:8080   https://localhost:8080
    Override LogConf File as Global Level  DEBUG
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    150
    ${contents} =  Get File  ${DEVICEISOLATION_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${LIVERESPONSE_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${RESPONSE_ACTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99

    Check Current Release With AV Installed Correctly

    Setup SUS static
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_normal_cloud_sub    ${fixed_version_name}    ${fixed_version_token}

    Remove File  ${tmpPolicy}
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}
    Register Cleanup  Remove File  ${tmpPolicy}

    ${update_mark} =  mark_log_size    ${UpdateSchedulerLog}
    ${sul_mark} =  mark_log_size  ${SULDOWNLOADER_LOG_PATH}
    Send Policy File  alc    ${tmpPolicy}
    wait_for_log_contains_from_mark  ${update_mark}    Using FixedVersion ${fixed_version_name} with token ${fixed_version_token}    20

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   90 secs
    ...   1 secs
    ...   File Should exist   ${UPGRADING_MARKER_FILE}
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   File Should not exist   ${UPGRADING_MARKER_FILE}


    wait_for_log_contains_from_mark  ${sul_mark}  Preparing ServerProtectionLinux-Base-component for downgrade    60

    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Base-component is being downgraded
    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Plugin-DeviceIsolation is being downgraded
    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Plugin-responseactions is being downgraded
    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Plugin-RuntimeDetections is being downgraded
    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Plugin-liveresponse is being downgraded
    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Plugin-EventJournaler is being downgraded
    wait_for_log_contains_from_mark  ${sul_mark}  Component ServerProtectionLinux-Plugin-EDR is being downgraded


    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Suldownloader Is Not Running

    Wait Until Keyword Succeeds
    ...   50 secs
    ...   10 secs
    ...  Check Installed Plugins Are VUT Versions

    Mark Known Upgrade Errors
    Mark Known Downgrade Errors

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical


Install all plugins static then upgrade to all plugins static-999
    [Tags]  BASE_DOWNGRADE  THIN_INSTALLER  INSTALLER  UNINSTALLER

    Setup SUS static
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_normal_cloud_sub    ${fixed_version_name}    ${fixed_version_token}

    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}
    Register Cleanup  Remove File  ${tmpPolicy}

    Start Local Cloud Server  --initial-alc-policy  ${tmpPolicy}

    Start Local SDDS3 Server    launchdarklyPath=${tmpLaunchDarkly}
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    configure_and_run_SDDS3_thininstaller  0  https://localhost:8080   https://localhost:8080
    wait_for_log_contains_from_mark    ${sul_mark}    Update success    150
    Check Current Release With AV Installed Correctly

    Setup SUS static 999
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_normal_cloud_sub    ${fixed_version_name}    ${fixed_version_token}

    Remove File  ${tmpPolicy}
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}
    Register Cleanup  Remove File  ${tmpPolicy}

    ${update_mark} =  mark_log_size    ${UpdateSchedulerLog}
    Send Policy File  alc    ${tmpPolicy}
    wait_for_log_contains_from_mark  ${update_mark}    Using FixedVersion ${fixed_version_name} with token ${fixed_version_token}    20

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   90 secs
    ...   1 secs
    ...   File Should exist   ${UPGRADING_MARKER_FILE}
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   File Should not exist   ${UPGRADING_MARKER_FILE}


    ${contents} =  Get File  ${DEVICEISOLATION_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${LIVERESPONSE_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${RESPONSE_ACTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Suldownloader Is Not Running
    Mark Known Upgrade Errors

    Check Current Release With AV Installed Correctly

*** Keywords ***
Check MCS Envelope Contains Event with Update cache
    [Arguments]  ${Event_Number}
    ${string}=  Check Log And Return Nth Occurrence Between Strings   <event><appId>ALC</appId>  </event>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Event_Number}
    Should contain   ${string}   updateSource&gt;4092822d-0925-4deb-9146-fbc8532f8c55&lt

