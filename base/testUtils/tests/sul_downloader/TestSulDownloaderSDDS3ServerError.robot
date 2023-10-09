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
...                Remove Environment Variable  COMMAND  AND
...                Remove Environment Variable  EXITCODE  AND
...                Upgrade Resources SDDS3 Test Teardown

Library    DateTime
Library     ${LIBS_DIRECTORY}/FakeSDDS3UpdateCacheUtils.py
Library     ${LIBS_DIRECTORY}/PolicyUtils.py

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

*** Test Cases ***
Sul Downloader Report error correctly when it cannot connect to sdds3
    Start Local Cloud Server
    ${handle}=  Start Local SDDS3 Server With Empty Repo
    Set Suite Variable    ${GL_handle}    ${handle}
    Require Fresh Install
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override  URLS=http://localhost:9000
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"
    Trigger Update Now
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    Check SulDownloader Log Contains  Failed to connect to repository: SUS request failed with error: Couldn't connect to server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    Check SulDownloader Log Contains  Update failed, with code: 107


Sul Downloader Reports Update Failed When Requested Fixed Version Is Unsupported In SDDS3
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicyForceSDDS2.xml
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
    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   File Should Contain    ${SOPHOS_INSTALL}/logs/base/suldownloader.log     The requested fixed version is not available on SDDS3: 1.1.0. Package too old.

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   File Should Contain    ${SOPHOS_INSTALL}/logs/base/suldownloader.log     Update failed, with code: 107

    Wait Until Keyword Succeeds
    ...   10 secs
    ...   2 secs
    ...   File Should Contain    ${SOPHOS_INSTALL}/logs/base/suldownloader.log     Generating the report file in:

Sul Downloader sdds3 sync Does not retry on curl errors
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files   port=8080
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override   CDN_URL=https://localhost:8090
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update failed
    check_log_does_not_contain    before retry after attempts:   ${SOPHOS_INSTALL}/logs/base/suldownloader_sync.log  sync
    check_log_contains    caught exception: Error performing http request: Couldn't connect to server, we will not retry this url   ${SOPHOS_INSTALL}/logs/base/suldownloader_sync.log  sync


SUS Fault Injection Server Hangs
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    Set Environment Variable  COMMAND   sus_hang
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

    Wait Until Keyword Succeeds
    ...   70 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request failed to connect to the server with error: Timeout was reached
        ...  Update failed, with code: 112


SUS Fault Injection Server Responds With Unauthorised
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    Set Environment Variable  COMMAND   sus_401
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

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request received HTTP response code: 401 but was expecting: 200
        ...  Update failed, with code: 107


SUS Fault Injection Server Responds With Not Found
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    Set Environment Variable  COMMAND   sus_404
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

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request received HTTP response code: 404 but was expecting: 200
        ...  Update failed, with code: 107


SUS Fault Injection Server Responds With Internal Server Error
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    Set Environment Variable  COMMAND   sus_500
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
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"

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
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request caused exception: Failed to parse SUS response
        ...  Update failed, with code: 107


SUS Fault Injection Server Responds With Large JSON
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
    Set Environment Variable  COMMAND   sus_large_json
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

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request failed with error: Transferred a partial file
        ...  Update failed, with code: 107

SUS Fault Injection Server response with static version not found
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_normal_cloud_sub    LTS 2023.1.1    f4d41a16-b751-4195-a7b2-1f109d49469d
    Create File  /tmp/tmpALC.xml   ${esm_enabled_alc_policy}
    Start Local Cloud Server    --initial-alc-policy  /tmp/tmpALC.xml
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

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: error: 'Did not return any suites' reason: 'Fixed version token not found' code: 'FIXED_VERSION_TOKEN_NOT_FOUND'
        ...  Update failed, with code: 107

CDN Fault Injection Does Not Contain Location Given By SUS
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Generate Warehouse From Local Base Input
    ${Files} =  List Files In Directory  ${SDDS3_FAKEPACKAGES}
    Remove File  ${SDDS3_FAKEPACKAGES}/${Files[0]}
    ${handle}=  Start Local SDDS3 server with fake files   port=8080
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override   CDN_URL=https://localhost:8080
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update failed
    Check SulDownloader Log Contains    Failed to synchronize repository
    Check SulDownloader Log Contains    : 404


CDN Fault Injection Server Responds With Unauthorised Error
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Set Environment Variable  EXITCODE   401
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files   port=8080
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update failed
    Check SulDownloader Log Contains    Failed to synchronize repository
    Check SulDownloader Log Contains    : 401


CDN Fault Injection Server Responds With Not Found Error
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Set Environment Variable  EXITCODE   404
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files   port=8080
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update failed
    Check SulDownloader Log Contains    Failed to synchronize repository
    Check SulDownloader Log Contains    : 404


CDN Fault Injection Server Responds With Generic Error
    Start Local Cloud Server  --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_policy_direct_just_base.xml
    Set Environment Variable  EXITCODE   500
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files   port=8080
    Set Suite Variable    ${GL_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override
    
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    File Should Contain  ${UPDATE_CONFIG}     "JWToken"

    Wait Until Keyword Succeeds
    ...    60s
    ...    5s
    ...    Check SulDownloader Log Contains  Update failed
    Check SulDownloader Log Contains    Failed to synchronize repository
    Check SulDownloader Log Contains    : 500

