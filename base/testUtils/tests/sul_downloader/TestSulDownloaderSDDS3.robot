*** Settings ***
Suite Setup      Suite Setup Without Ostia
Suite Teardown   Suite Teardown Without Ostia

Test Setup       Test Setup
Test Teardown    Run Keywords
...                Stop Local SDDS3 Server   AND
...                Remove Environment Variable  http_proxy    AND
...                Remove Environment Variable  https_proxy  AND
...                Stop Proxy If Running    AND
...                Stop Proxy Servers   AND
...                Clean up fake warehouse  AND
...                 Test Teardown


Library     ${LIBS_DIRECTORY}/FakeSDDS3UpdateCacheUtils.py

Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource    SulDownloaderResources.robot

Default Tags  SULDOWNLOADER


*** Variables ***
${sdds2_primary}                            ${SOPHOS_INSTALL}/base/update/cache/primary
${sdds2_primary_warehouse}                  ${SOPHOS_INSTALL}/base/update/cache/primarywarehouse
${sdds3_override_file}                      ${SOPHOS_INSTALL}/base/update/var/sdds3_override_settings.ini
${sdds3_server_output}                      /tmp/sdds3_server.log

*** Test Cases ***
Sul Downloader Requests Fixed Version When Fixed Version In Policy
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   File Should Contain    ${sdds3_server_output}     ServerProtectionLinux-Base fixedVersion: 2022.1.0.40 requested
    Wait Until Keyword Succeeds
    ...   2 secs
    ...   1 secs
    ...   File Should Contain    ${sdds3_server_output}     ServerProtectionLinux-Plugin-MDR fixedVersion: 1.0.2 requested

Sul Downloader Report error correctly when it cannot connect to sdds3
    Start Local Cloud Server
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override  URLS=http://localhost:9000
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    Check SulDownloader Log Contains  Failed to connect to repository: SUS request failed with error: Couldn't connect to server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    Check SulDownloader Log Contains  Update failed, with code: 112


Sul Downloader Forces SDDS2 When Requested Fixed Version Is Unsupported In SDD3
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicyForceSDDS2.xml
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   File Should Contain    ${SOPHOS_INSTALL}/logs/base/suldownloader.log     The requested fixed version is not available on SDDS3: 1.1.0. Reverting to SDDS2 mode.


Sul Downloader Uses Current Proxy File for SUS Requests
    Start Proxy Server With Basic Auth  3129  user  password
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    Log File  ${SOPHOS_INSTALL}/base/update/var/sdds3_override_settings.ini
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    Override LogConf File as Global Level  DEBUG
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken

    # Stop the product so that our current_proxy file doesn't get overridden by MCS.
    Run Process   systemctl  stop  sophos-spl

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
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
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

Sul Downloader Installs SDDS3 Through update cache
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override   CDN_URL=https://localhost:8081
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    Trigger Update Now
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


Sul Downloader falls back to direct when proxy and UC do not work
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
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    Trigger Update Now
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

Sul Downloader sdds3 sync Does not retry on curl errors
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files   port=8080
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override   CDN_URL=https://localhost:8090
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update failed
    check_log_does_not_contain    before retry after attempts:   ${SOPHOS_INSTALL}/logs/base/suldownloader_sync.log  sync
    check_log_contains    caught exception: Couldn't connect to server   ${SOPHOS_INSTALL}/logs/base/suldownloader_sync.log  sync


SUS Fault Injection Server Hangs
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    Set Environment Variable  COMMAND   sus_hang
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    Log File    ${SDDS3_OVERRIDE_FILE}
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    Override LogConf File as Global Level  DEBUG
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken

    # Make sure there isn't some old update report hanging around
    Run Keyword And Ignore Error   Remove File   ${TEST_TEMP_DIR}/update_report.json
    Create Directory  ${TEST_TEMP_DIR}

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request failed with error: Timeout was reached
        ...  Update failed, with code: 112


SUS Fault Injection Server Responds With Unauthorised
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    Set Environment Variable  COMMAND   sus_401
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    Log File    ${SDDS3_OVERRIDE_FILE}
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    Override LogConf File as Global Level  DEBUG
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken

    # Make sure there isn't some old update report hanging around
    Run Keyword And Ignore Error   Remove File   ${TEST_TEMP_DIR}/update_report.json
    Create Directory  ${TEST_TEMP_DIR}

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request received HTTP response code: 401 but was expecting: 200
        ...  Update failed, with code: 112


SUS Fault Injection Server Responds With Not Found
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    Set Environment Variable  COMMAND   sus_404
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    Log File    ${SDDS3_OVERRIDE_FILE}
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    Override LogConf File as Global Level  DEBUG
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken

    # Make sure there isn't some old update report hanging around
    Run Keyword And Ignore Error   Remove File   ${TEST_TEMP_DIR}/update_report.json
    Create Directory  ${TEST_TEMP_DIR}

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request received HTTP response code: 404 but was expecting: 200
        ...  Update failed, with code: 112


SUS Fault Injection Server Responds With Internal Server Error
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    Set Environment Variable  COMMAND   sus_500
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    Log File    ${SDDS3_OVERRIDE_FILE}
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    Override LogConf File as Global Level  DEBUG
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken

    # Make sure there isn't some old update report hanging around
    Run Keyword And Ignore Error   Remove File   ${TEST_TEMP_DIR}/update_report.json
    Create Directory  ${TEST_TEMP_DIR}

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request received HTTP response code: 500 but was expecting: 200
        ...  Update failed, with code: 112


SUS Fault Injection Server Responds With Service Unavailable
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    Set Environment Variable  COMMAND   sus_503
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    Log File    ${SDDS3_OVERRIDE_FILE}
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    Override LogConf File as Global Level  DEBUG
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken

    # Make sure there isn't some old update report hanging around
    Run Keyword And Ignore Error   Remove File   ${TEST_TEMP_DIR}/update_report.json
    Create Directory  ${TEST_TEMP_DIR}

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request received HTTP response code: 503 but was expecting: 200
        ...  Update failed, with code: 112


SUS Fault Injection Server Responds With Invalid JSON
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    Set Environment Variable  COMMAND   sus_invalid_json
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    Log File    ${SDDS3_OVERRIDE_FILE}
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    Override LogConf File as Global Level  DEBUG
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken

    # Make sure there isn't some old update report hanging around
    Run Keyword And Ignore Error   Remove File   ${TEST_TEMP_DIR}/update_report.json
    Create Directory  ${TEST_TEMP_DIR}

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request caused exception: Failed to parse SUS response
        ...  Update failed, with code: 112


SUS Fault Injection Server Responds With Large JSON
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    Set Environment Variable  COMMAND   sus_large_json
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    Log File    ${SDDS3_OVERRIDE_FILE}
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    Override LogConf File as Global Level  DEBUG
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken

    # Make sure there isn't some old update report hanging around
    Run Keyword And Ignore Error   Remove File   ${TEST_TEMP_DIR}/update_report.json
    Create Directory  ${TEST_TEMP_DIR}

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request failed with error: Transferred a partial file
        ...  Update failed, with code: 112


CDN Fault Injection Does Not Contain Location Given By SUS
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files   port=8080
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override   CDN_URL=https://localhost:8080
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken

    Replace Local Launchdarkly Json Content With Fake Suite
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update failed
    Check SulDownloader Log Contains    Failed to synchronize repository
    Check SulDownloader Log Contains    : 404


CDN Fault Injection Server Responds With Unauthorised Error
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Set Environment Variable  COMMAND   cdn_401
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files   port=8080
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update failed
    Check SulDownloader Log Contains    Failed to synchronize repository
    Check SulDownloader Log Contains    : 401


CDN Fault Injection Server Responds With Not Found Error
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Set Environment Variable  COMMAND   cdn_404
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files   port=8080
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update failed
    Check SulDownloader Log Contains    Failed to synchronize repository
    Check SulDownloader Log Contains    : 404


CDN Fault Injection Server Responds With Generic Error
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Set Environment Variable  COMMAND   cdn_500
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files   port=8080
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    5s
    ...    1s
    ...    Log File    ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    Trigger Update Now

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update failed
    Check SulDownloader Log Contains    Failed to synchronize repository
    Check SulDownloader Log Contains    : 500


*** Keywords ***
Check MCS Envelope Contains Event with Update cache
    [Arguments]  ${Event_Number}
    ${string}=  Check Log And Return Nth Occurence Between Strings   <event><appId>ALC</appId>  </event>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Event_Number}
    Should contain   ${string}   updateSource&gt;4092822d-0925-4deb-9146-fbc8532f8c55&lt

File Should Contain
    [Arguments]  ${file_path}  ${expected_contents}
    ${contents}=  Get File   ${file_path}
    Should Contain  ${contents}   ${expected_contents}
