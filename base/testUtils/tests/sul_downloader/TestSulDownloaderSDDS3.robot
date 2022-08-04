*** Settings ***
Suite Setup      Suite Setup Without Ostia
Suite Teardown   Suite Teardown Without Ostia

Test Setup       Test Setup
Test Teardown    Run Keywords
...                Stop Local SDDS3 Server  AND
...                Stop Proxy If Running    AND
...                Test Teardown

Test Timeout  10 mins

Resource    ../installer/InstallerResources.robot
Resource    ../upgrade_product/UpgradeResources.robot

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
    ...   File Should Contain    ${sdds3_server_output}     ServerProtectionLinux-Base fixedVersion: 1.2.0 requested
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


*** Keywords ***
File Should Contain
    [Arguments]  ${file_path}  ${expected_contents}
    ${contents}=  Get File   ${file_path}
    Should Contain  ${contents}   ${expected_contents}
