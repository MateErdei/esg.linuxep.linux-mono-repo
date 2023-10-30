*** Settings ***
Suite Setup      Upgrade Resources Suite Setup
Suite Teardown   Upgrade Resources Suite Teardown

Test Setup       Require Uninstalled
Test Teardown    Run Keywords
...                Remove Environment Variable  http_proxy    AND
...                Remove Environment Variable  https_proxy  AND
...                Stop Proxy If Running    AND
...                Stop Proxy Servers   AND
...                Remove File  /tmp/tmpALC.xml   AND
...                Clean up fake warehouse  AND
...                Upgrade Resources SDDS3 Test Teardown

Library     DateTime
Library     ${COMMON_TEST_LIBS}/FakeSDDS3UpdateCacheUtils.py
Library     ${COMMON_TEST_LIBS}/PolicyUtils.py

Resource    ${COMMON_TEST_ROBOT}/GeneralUtilsResources.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/SchedulerUpdateResources.robot
Resource    ${COMMON_TEST_ROBOT}/SDDS3Resources.robot
Resource    ${COMMON_TEST_ROBOT}/SulDownloaderResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Force Tags  SULDOWNLOADER  TAP_PARALLEL1

*** Variables ***
${sdds3_server_output}                      ${sdds3_server_log}

*** Test Cases ***
Sul Downloader Requests Correct Package For Machine Architecture
    ${platform} =    machine_architecture
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override

    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"
    ${sul_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   File Should Contain    ${sdds3_server_output}     Package requested for ${platform}
    wait_for_log_contains_from_mark  ${sul_mark}  Doing product and supplement update

Sul Downloader Requests Fixed Version When Fixed Version In Policy
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"
    ${sul_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   File Should Contain    ${sdds3_server_output}     ServerProtectionLinux-Base fixedVersion: 2022.1.0.40 requested
    wait_for_log_contains_from_mark  ${sul_mark}  Doing product and supplement update

Update Now action triggers a product update even when updates are scheduled
    ${BasicPolicyXml} =  Get File  ${SUPPORT_FILES}/CentralXml/ALC_policy_scheduled_update.xml
    ${Date} =  Get Current Date
    ${ScheduledDate} =  Add Time To Date  ${Date}  60 minutes
    ${ScheduledDay} =  Convert Date  ${ScheduledDate}  result_format=%A
    ${ScheduledTime} =  Convert Date  ${ScheduledDate}  result_format=%H:%M:00
    ${NewPolicyXml} =  Replace String  ${BasicPolicyXml}  REPLACE_DAY  ${ScheduledDay}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  REPLACE_TIME  ${ScheduledTime}
    ${NewPolicyXml} =  Replace String  ${NewPolicyXml}  Frequency="40"  Frequency="7"
    Create File  /tmp/ALC_policy_scheduled_update.xml  ${NewPolicyXml}
    Log File  /tmp/ALC_policy_scheduled_update.xml


    Start Local Cloud Server    --initial-alc-policy  /tmp/ALC_policy_scheduled_update.xml
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    ${sul_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"
    ${sul_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Trigger Update Now
    wait_for_log_contains_from_mark  ${sul_mark}  Doing product and supplement update

Sul Downloader Uses Current Proxy File for SUS Requests
    Start Proxy Server With Basic Auth  3129  user  password
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    Setup Dev Certs For SDDS3
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"
    Override LogConf File as Global Level  DEBUG

    # Stop the product so that our current_proxy file doesn't get overridden by MCS.
    Run Process   systemctl  stop  sophos-spl
    Run Process   systemctl  stop  sophos-spl-update

    # Obfuscated credentials are "user", "password"
    Create File  ${CURRENT_PROXY_FILE}   {"proxy": "localhost:3129", "credentials": "CCBv6oin2yWCd1PUWKpab1GcYXBB0iC1bwnajy0O1XVvOrRTTFGiruMEz5auCd8BpbE="}
    Log File  ${CURRENT_PROXY_FILE}

    # Make sure there isn't some old update report hanging around
    Run Keyword And Ignore Error   Remove File   ${TEST_TEMP_DIR}/update_report.json
    Create Directory  ${TEST_TEMP_DIR}

    # Run suldownloader
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${UPDATE_CONFIG}    ${TEST_TEMP_DIR}/update_report.json

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Trying SUS request (https://localhost:8080) with proxy: localhost:3129
        ...  SUS request received HTTP response code: 200
        ...  SUS Request was successful


Sul Downloader Installs SDDS3 Through Proxy
    # TODO: LINUXDAR-8281: Fix and re-enable local SDDS3 server system tests
    [Tags]  DISABLED
    Start Simple Proxy Server    1235
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    ${proxy_url} =    Set Variable    http://localhost:1235/
    Set Environment Variable  http_proxy   ${proxy_url}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    20s
    ...    5s
    ...    Check SulDownloader Log Contains  Trying to update via proxy localhost:1235 to https://localhost:8080
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update success
    Check Sul Downloader log does not contain    Connecting to update source directly
    Log File  ${SOPHOS_INSTALL}/base/pluginRegistry/updatescheduler.json

SDDS3 updates supplements
    # TODO: LINUXDAR-8281: Fix and re-enable local SDDS3 server system tests
    [Tags]  DISABLED
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup Install SDDS3 Base

    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    
    Register With Local Cloud Server

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update success

    File Should Not Contain  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  "tests"
    Generate Warehouse From Local Base Input  "{\"tests\":\"false\"}"
    Setup Dev Certs for sdds3
    ${sul_mark} =  mark_log_size  ${SULDOWNLOADER_LOG_PATH}
    Trigger Update Now
    wait_for_log_contains_from_mark  ${sul_mark}  Update success      80
    File Should Contain  ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json  tests


Sul Downloader Installs SDDS3 Through update cache
    # TODO: LINUXDAR-8281: Fix and re-enable local SDDS3 server system tests
    [Tags]  DISABLED
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override   CDN_URL=https://localhost:8081
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"
    Wait Until Keyword Succeeds
    ...    20s
    ...    5s
    ...    Check SulDownloader Log Contains  Trying update via update cache: https://localhost:8080
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update success
    Check Sul Downloader log does not contain    Connecting to update source directly
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check MCS Envelope Contains Event with Update cache   1

Sul Downloader Installs SDDS3 Through message relay
    # TODO: LINUXDAR-8281: Fix and re-enable local SDDS3 server system tests
    [Tags]  DISABLED
    Start Simple Proxy Server    3333

    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-mcs-policy  ${SUPPORT_FILES}/CentralXml/MCS_policy_with_proxy.xml    --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag

    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}
    Create Local SDDS3 Override   CDN_URL=https://localhost:8081

    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"

    wait_for_log_contains_from_mark    ${sul_mark}    Trying SUS request (https://localhost:8080) with proxy: localhost:3333   timeout=20
    wait_for_log_contains_from_mark    ${sul_mark}    SUS Request was successful    timeout=20
    wait_for_log_contains_from_mark    ${sul_mark}    Trying update via update cache: https://localhost:8080     timeout=20
    wait_for_log_contains_from_mark    ${sul_mark}    Update success     timeout=60

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check MCS Envelope Contains Event with Update cache   1

    check_suldownloader_log_should_not_contain     Trying SUS request (https://localhost:8080) without proxy



Sul Downloader falls back to direct when proxy and UC do not work
    # TODO: LINUXDAR-8281: Fix and re-enable local SDDS3 server system tests
    [Tags]  DISABLED
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files   port=8081
    Set Suite Variable    ${GL_handle}    ${handle}
    ${proxy_url} =    Set Variable    http://localhost:1235/
    Set Environment Variable  http_proxy   ${proxy_url}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override   CDN_URL=https://localhost:8081  URLS=https://localhost:8081
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"
    Wait Until Keyword Succeeds
    ...    20s
    ...    5s
    ...    Check SulDownloader Log Contains  Trying SUS request (https://localhost:8081) with proxy: http://localhost:1235/
    Wait Until Keyword Succeeds
    ...    20s
    ...    5s
    ...    Check SulDownloader Log Contains  Trying SUS request (https://localhost:8081) without proxy
    Wait Until Keyword Succeeds
    ...    20s
    ...    5s
    ...    Check SulDownloader Log Contains  Trying update via update cache: https://localhost:8080
    Wait Until Keyword Succeeds
    ...    20s
    ...    5s
    ...    Check SulDownloader Log Contains  Trying to update via proxy http://localhost:1235/ to https://localhost:8081
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update success
    Check SulDownloader Log Contains    Connecting to update source directly

Sul Downloader sets sdds3 v2 delta if flag present
    # TODO: LINUXDAR-8281: Fix and re-enable local SDDS3 server system tests
    [Tags]  DISABLED
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Generate Warehouse From Local Base Input  {"sdds3.delta-versioning.enabled": "true"}
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    ${sul_mark} =    mark_log_size    ${SULDOWNLOADER_LOG_PATH}

    Create Local SDDS3 Override
    Register With Local Cloud Server

    wait_for_log_contains_from_mark    ${sul_mark}    Update success    120
    Log File    ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    File Should Contain    ${SOPHOS_INSTALL}/base/mcs/policy/flags.json    sdds3.delta-versioning.enabled": true
    Override LogConf File as Global Level  DEBUG
    Trigger Update Now
    wait_for_log_contains_from_mark    ${sul_mark}    Enabling sdds3 delta V2 usage    40

*** Keywords ***
Check MCS Envelope Contains Event with Update cache
    [Arguments]  ${Event_Number}
    ${string}=  Check Log And Return Nth Occurrence Between Strings   <event><appId>ALC</appId>  </event>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Event_Number}
    Should contain   ${string}   updateSource&gt;4092822d-0925-4deb-9146-fbc8532f8c55&lt

