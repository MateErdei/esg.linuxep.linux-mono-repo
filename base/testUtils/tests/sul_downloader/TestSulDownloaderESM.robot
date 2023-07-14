*** Settings ***
Suite Setup      Upgrade Resources Suite Setup
Suite Teardown   Upgrade Resources Suite Teardown

Test Setup       Upgrade Resources Test Setup
Test Teardown    Run Keywords
...                Remove Environment Variable  http_proxy    AND
...                Remove Environment Variable  https_proxy  AND
...                Stop Proxy If Running    AND
...                Stop Proxy Servers   AND
...                Clean up fake warehouse  AND
...                Remove Environment Variable  COMMAND  AND
...                Remove Environment Variable  EXITCODE  AND
...                Upgrade Resources SDDS3 Test Teardown

Library    DateTime
Library     ${LIBS_DIRECTORY}/FakeSDDS3UpdateCacheUtils.py
Library     ${LIBS_DIRECTORY}/PolicyUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py

Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../update/SDDS3Resources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource    SulDownloaderResources.robot
Resource  ../GeneralUtilsResources.robot

Default Tags  SULDOWNLOADER
Force Tags  LOAD6

*** Variables ***
${sdds3_server_output}                      /tmp/sdds3_server.log
${UpdateSchedulerLog}                       ${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log
${tmpPolicy} =                               /tmp/tmpALC.xml
${tmpLaunchDarkly} =                        /tmp/launchdarkly
${staticflagfile} =                         linuxep.json
${SULDownloaderLogDowngrade}        ${SOPHOS_INSTALL}/logs/base/downgrade-backup/suldownloader.log


*** Test Cases ***
Fixed Version Token is requested by SulDownloader
    ${esm_enabled_alc_policy} =    populate_esm_fixed_version    LTS 2023.1.1    f4d41a16-b751-4195-a7b2-1f109d49469d
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}
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

    wait_for_log_contains_from_mark  ${update_mark}  Using FixedVersion LTS 2023.1.1 with token f4d41a16-b751-4195-a7b2-1f109d49469d

    File Should Contain  ${UPDATE_CONFIG}     JWToken
    ${sul_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/suldownloader.log

    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   File Should Contain    ${sdds3_server_output}     fixed_version_token f4d41a16-b751-4195-a7b2-1f109d49469d
    wait_for_log_contains_from_mark  ${sul_mark}  Doing product and supplement update


Request is not made by SulDownloader when ESM is invalid
    #Invalid because we are only providing a name for the fixed_version
    ${esm_enabled_alc_policy} =    populate_esm_fixed_version    LTS 2023.1.1
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

    ${sul_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/suldownloader.log

    Trigger Update Now
    wait_for_log_contains_from_mark  ${sul_mark}   ESM feature is not valid. Name: LTS 2023.1.1 and Token:


Fixed Version Token is not requested by SulDownloader when ESM not present in ALC
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



Install all plugins static-999 then downgrade to all plugins static
    [Tags]  BASE_DOWNGRADE  THIN_INSTALLER  INSTALLER  UNINSTALLER  EXCLUDE_SLES12  EXCLUDE_SLES15

    Setup SUS static 999
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_esm_fixed_version    ${fixed_version_name}    ${fixed_version_token}
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}
    Register Cleanup  Remove File  ${tmpPolicy}

    Start Local Cloud Server  --initial-alc-policy  ${tmpPolicy}

    # LINUXDAR-7059: On SUSE the thin installer fails to connect to the first SDDS3 server so workaround for now by running twice
    ${result} =  Run Process    bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIBS_DIRECTORY}/SDDS3server.py --launchdarkly ${tmpLaunchDarkly} --sdds3 ${VUT_WAREHOUSE_ROOT}/repo  shell=true    timeout=10s
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIBS_DIRECTORY}/SDDS3server.py --launchdarkly ${tmpLaunchDarkly} --sdds3 ${VUT_WAREHOUSE_ROOT}/repo  shell=true

    Set Suite Variable    ${GL_handle}    ${handle}

    configure_and_run_SDDS3_thininstaller  0  https://localhost:8080   https://localhost:8080
    Override LogConf File as Global Level  DEBUG
    Wait Until Keyword Succeeds
    ...   150 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2
    ${contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 9.99.9
    ${contents} =  Get File  ${MTR_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 9.99.9
    ${contents} =  Get File  ${LIVERESPONSE_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 9.99.9
    ${contents} =  Get File  ${RESPONSE_ACTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.9.9.999
    ${contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 999.999.999

    Check Current Release With AV Installed Correctly

    Setup SUS static
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_esm_fixed_version    ${fixed_version_name}    ${fixed_version_token}

    Remove File  ${tmpPolicy}
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}
    Register Cleanup  Remove File  ${tmpPolicy}
    Send Policy File  alc    ${tmpPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   90 secs
    ...   1 secs
    ...   File Should exist   ${UPGRADING_MARKER_FILE}
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   File Should not exist   ${UPGRADING_MARKER_FILE}
    Wait Until Keyword Succeeds
    ...   200 secs
    ...   10 secs
    ...   Directory Should Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup

    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check Log Contains  Preparing ServerProtectionLinux-Base-component for downgrade  ${SULDownloaderLogDowngrade}  backedup suldownloader log

    Check Log Contains  Component ServerProtectionLinux-Base-component is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Contains  Component ServerProtectionLinux-Plugin-responseactions is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Contains  Component ServerProtectionLinux-Plugin-RuntimeDetections is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Contains  Component ServerProtectionLinux-Plugin-liveresponse is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Contains  Component ServerProtectionLinux-Plugin-EventJournaler is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Contains  Component ServerProtectionLinux-Plugin-EDR is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log
    Check Log Contains  Component ServerProtectionLinux-Plugin-MDR is being downgraded   ${SULDownloaderLogDowngrade}  backedup suldownloader log


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


Install all plugins static then uppgrade to all plugins static-999
    [Tags]  BASE_DOWNGRADE  THIN_INSTALLER  INSTALLER  UNINSTALLER  EXCLUDE_SLES12  EXCLUDE_SLES15

    Setup SUS static
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_esm_fixed_version    ${fixed_version_name}    ${fixed_version_token}

    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}
    Register Cleanup  Remove File  ${tmpPolicy}

    Start Local Cloud Server  --initial-alc-policy  ${tmpPolicy}

    # LINUXDAR-7059: On SUSE the thin installer fails to connect to the first SDDS3 server so workaround for now by running twice
    ${result} =  Run Process    bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIBS_DIRECTORY}/SDDS3server.py --launchdarkly ${tmpLaunchDarkly} --sdds3 ${VUT_WAREHOUSE_ROOT}/repo  shell=true    timeout=10s
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${LIBS_DIRECTORY}/SDDS3server.py --launchdarkly ${tmpLaunchDarkly} --sdds3 ${VUT_WAREHOUSE_ROOT}/repo  shell=true

    Set Suite Variable    ${GL_handle}    ${handle}

    configure_and_run_SDDS3_thininstaller  0  https://localhost:8080   https://localhost:8080
    Wait Until Keyword Succeeds
    ...   80 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SOPHOS_INSTALL}/logs/base/suldownloader.log   suldownloader_log   Update success  2
    Check Current Release With AV Installed Correctly

    Setup SUS static 999
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_esm_fixed_version    ${fixed_version_name}    ${fixed_version_token}

    Remove File  ${tmpPolicy}
    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}
    Register Cleanup  Remove File  ${tmpPolicy}
    Send Policy File  alc    ${tmpPolicy}
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   90 secs
    ...   1 secs
    ...   File Should exist   ${UPGRADING_MARKER_FILE}
    Wait Until Keyword Succeeds
    ...   300 secs
    ...   10 secs
    ...   File Should not exist   ${UPGRADING_MARKER_FILE}


    ${contents} =  Get File  ${EDR_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 9.99.9
    ${contents} =  Get File  ${MTR_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 9.99.9
    ${contents} =  Get File  ${LIVERESPONSE_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.99.99
    ${contents} =  Get File  ${EVENTJOURNALER_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 9.99.9
    ${contents} =  Get File  ${RESPONSE_ACTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 99.9.9.999
    ${contents} =  Get File  ${RUNTIMEDETECTIONS_DIR}/VERSION.ini
    Should contain   ${contents}   PRODUCT_VERSION = 999.999.999

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

Setup SUS static
    Remove Directory   ${tmpLaunchDarkly}   recursive=True
    Create Directory   ${tmpLaunchDarkly}
    Copy File  ${VUT_WAREHOUSE_ROOT}/launchdarkly-static/linuxep.json   ${tmpLaunchDarkly}

    Copy File  ${VUT_WAREHOUSE_ROOT}/launchdarkly/release.linuxep.ServerProtectionLinux-Base.json   ${tmpLaunchDarkly}
    Copy File  ${VUT_WAREHOUSE_ROOT}/launchdarkly/release.linuxep.ServerProtectionLinux-Plugin-AV.json   ${tmpLaunchDarkly}
    Copy File  ${VUT_WAREHOUSE_ROOT}/launchdarkly/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  ${tmpLaunchDarkly}
    Copy File  ${VUT_WAREHOUSE_ROOT}/launchdarkly/release.linuxep.ServerProtectionLinux-Plugin-MDR.json  ${tmpLaunchDarkly}


Setup SUS static 999
    Remove Directory   ${tmpLaunchDarkly}   recursive=True
    Create Directory   ${tmpLaunchDarkly}
    Copy File  ${VUT_WAREHOUSE_ROOT}/launchdarkly-static-999/linuxep.json   ${tmpLaunchDarkly}

    Copy File  ${VUT_WAREHOUSE_ROOT}/launchdarkly-999/release.linuxep.ServerProtectionLinux-Base.json   ${tmpLaunchDarkly}
    Copy File  ${VUT_WAREHOUSE_ROOT}/launchdarkly-999/release.linuxep.ServerProtectionLinux-Plugin-AV.json   ${tmpLaunchDarkly}
    Copy File  ${VUT_WAREHOUSE_ROOT}/launchdarkly-999/release.linuxep.ServerProtectionLinux-Plugin-EDR.json  ${tmpLaunchDarkly}
    Copy File  ${VUT_WAREHOUSE_ROOT}/launchdarkly-999/release.linuxep.ServerProtectionLinux-Plugin-MDR.json  ${tmpLaunchDarkly}
