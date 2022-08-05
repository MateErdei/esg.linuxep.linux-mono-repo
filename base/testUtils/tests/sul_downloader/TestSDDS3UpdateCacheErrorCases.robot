*** Settings ***
Suite Setup      Suite Setup Without Ostia
Suite Teardown   Suite Teardown Without Ostia

Test Setup       Test Setup
Test Teardown    UC Error Test Teardown


Test Timeout  10 mins

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${LIBS_DIRECTORY}/TeardownTools.py
Library     ${LIBS_DIRECTORY}/UpgradeUtils.py
Library     ${LIBS_DIRECTORY}/FakeSDDS3UpdateCacheUtils.py

Library     Process
Library     OperatingSystem
Library     String


Resource    ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource    SulDownloaderResources.robot

Default Tags  SULDOWNLOADER
*** Keywords ***
UC Error Test Teardown
    terminate process  ${GL_UC_handle}  True
    Stop Local SDDS3 Server
    Clean up fake warehouse
    Remove Environment Variable   EXITCODE
    Remove Environment Variable   COMMAND
    Test Teardown


*** Variables ***
${fixed_version_policy}                     ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicy.xml

${status_file}                              ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

${sdds3_override_file}                      ${SOPHOS_INSTALL}/base/update/var/sdds3_override_settings.ini
${sdds3_server_output}                      /tmp/sdds3_server.log

*** Test Cases ***
Test that SDDS3 can Handle 202s from Update Caches
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files  8081
    Set Suite Variable    ${GL_handle}    ${handle}
    Set Environment Variable  EXITCODE  202
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_UC_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override  CDN_URL=https://localhost:8081

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
    ...    400s
    ...    30s
    ...    Check SulDownloader Log Contains  Update success
    Check SulDownloader Log Contains   Failed to Sync with https://localhost:8080 error: Error syncing https://localhost:8080/suite/sdds3.ServerProtectionLinux-Base_2022.7.22.7.020fb0c370.dat: 202

Test that SDDS3 can Handle 404s from Update Caches
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files  8081
    Set Suite Variable    ${GL_handle}    ${handle}
    Set Environment Variable  EXITCODE  404
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_UC_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override  CDN_URL=https://localhost:8081

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
    ...    50s
    ...    5s
    ...    Check SulDownloader Log Contains  Update success
    Check SulDownloader Log Contains   Failed to Sync with https://localhost:8080 error: Error syncing https://localhost:8080/suite/sdds3.ServerProtectionLinux-Base_2022.7.22.7.020fb0c370.dat: 404


Test that SDDS3 can Handle 202 then 404s from Update Caches
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files  8081
    Set Suite Variable    ${GL_handle}    ${handle}
    Set Environment Variable  COMMAND   failure
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_UC_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override  CDN_URL=https://localhost:8081

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
    ...    50s
    ...    5s
    ...    Check SulDownloader Log Contains  Update success
    Check SulDownloader Log Contains   Failed to Sync with https://localhost:8080 error: Error syncing https://localhost:8080/suite/sdds3.ServerProtectionLinux-Base_2022.7.22.7.020fb0c370.dat: 404


Test that SDDS3 can Handle 202 then success from Update Caches
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files  8081
    Set Suite Variable    ${GL_handle}    ${handle}
    Set Environment Variable  COMMAND  retry
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_UC_handle}    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override  CDN_URL=https://localhost:8081

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
    ...    100s
    ...    10s
    ...    Check SulDownloader Log Contains  Update success
    Check Sul Downloader log does not contain    Connecting to update source directly