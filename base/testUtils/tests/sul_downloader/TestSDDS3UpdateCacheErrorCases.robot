*** Settings ***
Suite Setup      Upgrade Resources Suite Setup
Suite Teardown   Upgrade Resources Suite Teardown

Test Setup       Upgrade Resources Test Setup
Test Teardown    UC Error Test Teardown

Force Tags  LOAD6
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
Resource    ../update/SDDS3Resources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource    SulDownloaderResources.robot

Default Tags  SULDOWNLOADER  sdds3updatecache_error_cases
*** Keywords ***
UC Error Test Teardown
    run keyword if  "${GL_UC_handle}" != "${EMPTY}"  terminate process  ${GL_UC_handle}  True
    Set Suite Variable    $GL_UC_handle    ${EMPTY}
    Stop Local SDDS3 Server
    Clean up fake warehouse
    Remove Environment Variable   EXITCODE
    Remove Environment Variable   COMMAND
    Upgrade Resources Test Teardown


*** Variables ***
${fixed_version_policy}                     ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicy.xml

${status_file}                              ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

${sdds3_server_output}                      /tmp/sdds3_server.log

*** Test Cases ***
Test that SDDS3 can Handle 202s from Update Caches
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files  8081
    Set Suite Variable    $GL_handle    ${handle}
    Set Environment Variable  EXITCODE  202
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    $GL_UC_handle    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override  CDN_URL=https://localhost:8081

    Register With Local Cloud Server
    wait until created  ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}   JWToken
    ${mark} =  mark_log_size  ${SUL_DOWNLOADER_LOG}
    Trigger Update Now
    wait_for_log_contains_from_mark  ${mark}  Trying update via update cache: https://localhost:8080  timeout=${20}
    wait_for_log_contains_from_mark  ${mark}  Update success  timeout=${400}
    wait_for_log_contains_from_mark  ${mark}  Failed to Sync with https://localhost:8080/v3 error: Error syncing https://localhost:8080/v3/suite/sdds3.ServerProtectionLinux-Base_2022.7.22.7.020fb0c370.dat: 202

Test that SDDS3 can Handle 404s from Update Caches
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files  8081
    Set Suite Variable    $GL_handle    ${handle}
    Set Environment Variable  EXITCODE  404
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    $GL_UC_handle    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override  CDN_URL=https://localhost:8081

    Register With Local Cloud Server
    wait until created  ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    ${mark} =  mark_log_size  ${SUL_DOWNLOADER_LOG}
    Trigger Update Now
    wait_for_log_contains_from_mark  ${mark}  Trying update via update cache: https://localhost:8080  timeout=${20}
    wait_for_log_contains_from_mark  ${mark}  Update success  timeout=${50}
    wait_for_log_contains_from_mark  ${mark}  Failed to Sync with https://localhost:8080/v3 error: Error syncing https://localhost:8080/v3/suite/sdds3.ServerProtectionLinux-Base_2022.7.22.7.020fb0c370.dat: 404


Test that SDDS3 can Handle 202 then 404s from Update Caches
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files  8081
    Set Suite Variable    $GL_handle    ${handle}
    Set Environment Variable  COMMAND   failure
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    $GL_UC_handle    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override  CDN_URL=https://localhost:8081

    Register With Local Cloud Server
    wait until created  ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    ${mark} =  mark_log_size  ${SUL_DOWNLOADER_LOG}
    Trigger Update Now
    wait_for_log_contains_from_mark  ${mark}  Trying update via update cache: https://localhost:8080  timeout=${20}
    wait_for_log_contains_from_mark  ${mark}  Update success  timeout=${50}
    Check SulDownloader Log Contains   Failed to Sync with https://localhost:8080/v3 error: Error syncing https://localhost:8080/v3/suite/sdds3.ServerProtectionLinux-Base_2022.7.22.7.020fb0c370.dat: 404


Test that SDDS3 can Handle 202 then success from Update Caches
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files  8081
    Set Suite Variable    $GL_handle    ${handle}
    Set Environment Variable  COMMAND  retry
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    $GL_UC_handle    ${handle}
    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override  CDN_URL=https://localhost:8081

    Register With Local Cloud Server
    wait until created  ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    ${mark} =  mark_log_size  ${SUL_DOWNLOADER_LOG}
    Trigger Update Now
    wait_for_log_contains_from_mark  ${mark}  Trying update via update cache: https://localhost:8080  timeout=${20}
    wait_for_log_contains_from_mark  ${mark}  Update success  timeout=${100}
    Check Sul Downloader log does not contain    Connecting to update source directly


Sul Downloader fails to Installs SDDS3 Through update cache if UC cert is wrong
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/CloudAutomation/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files  8081
    Set Suite Variable    $GL_handle    ${handle}
    ${handle}=  Start Local SDDS3 server with fake files   8080
    Set Suite Variable    $GL_UC_handle    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override   CDN_URL=https://localhost:8081   URLS=https://localhost:8081
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    wait until created  ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    ${mark} =  mark_log_size  ${SUL_DOWNLOADER_LOG}
    Trigger Update Now
    wait_for_log_contains_from_mark  ${mark}  Trying update via update cache: https://localhost:8080  timeout=${20}
    wait_for_log_contains_from_mark  ${mark}  Update success  timeout=${60}
    Check SulDownloader Log Contains    Failed to Sync with https://localhost:8080/v3 error: Error syncing https://localhost:8080/v3/suite/sdds3.ServerProtectionLinux-Base_2022.7.22.7.020fb0c370.dat: 0
    Check SulDownloader Log Contains    Connecting to update source directly

Sul Downloader fails back to direct if UC fails to install supplements
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files  8081
    Set Suite Variable    $GL_handle    ${handle}
    ${handle}=  Start Local SDDS3 server with fake files   8080
    Set Suite Variable    $GL_UC_handle    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override   CDN_URL=https://localhost:8081   URLS=https://localhost:8081
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    wait until created  ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    ${mark} =  mark_log_size  ${SUL_DOWNLOADER_LOG}
    Trigger Update Now
    wait_for_log_contains_from_mark  ${mark}  Trying update via update cache: https://localhost:8080  timeout=${20}
    wait_for_log_contains_from_mark  ${mark}  Update success  timeout=${60}
    Check Sul Downloader log does not contain    Connecting to update source directly
    Remove file   ${SDDS3_FAKESUPPLEMENT}/sdds3.SSPLFLAGS.dat
    Remove File    ${SOPHOS_INSTALL}/base/update/rootcerts/rootca384.crt.1
    Copy File  ${SUPPORT_FILES}/sophos_certs/rootca384.crt  ${SOPHOS_INSTALL}/base/update/rootcerts/rootca384.crt.1
    ${mark} =  mark_log_size  ${SUL_DOWNLOADER_LOG}
    Trigger Update Now
    wait_for_log_contains_from_mark  ${mark}  Connecting to update source directly  timeout=${50}
    Wait Until Keyword Succeeds
    ...    20s
    ...    5s
    ...    Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader.log  suldownloader_log   Update failed, with code: 107  1
    Check Log Contains String N times   ${SOPHOS_INSTALL}/logs/base/suldownloader_sync.log   suldownloader_sync_log   Failed to refresh supplement: falling back to cached supplement: Error syncing https://localhost:8081/supplement/sdds3.SSPLFLAGS.dat  1


Sul Downloader fails update if it cannot download fresh supplements
    write_ALC_update_cache_policy   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem
    Start Local Cloud Server  --initial-alc-policy  /tmp/ALC_policy.xml
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files  8081
    Set Suite Variable    ${GL_handle}    ${handle}
    ${handle}=  Start Local SDDS3 server with fake files   8080
    Set Suite Variable    ${GL_UC_handle}    ${handle}

    Setup Install SDDS3 Base
    Create File    ${MCS_DIR}/certs/ca_env_override_flag
    Create Local SDDS3 Override   CDN_URL=https://localhost:8081   URLS=https://localhost:8081
    # should be purged before SDDS3 sync
    Register With Local Cloud Server
    wait until created  ${UPDATE_CONFIG}
    ${content}=  Get File    ${UPDATE_CONFIG}
    File Should Contain  ${UPDATE_CONFIG}     JWToken
    Remove file   ${SDDS3_FAKESUPPLEMENT}/sdds3.SSPLFLAGS.dat
    ${mark} =  mark_log_size  ${SUL_DOWNLOADER_LOG}
    Trigger Update Now
    wait_for_log_contains_from_mark  ${mark}  Trying update via update cache: https://localhost:8080  timeout=${20}
    wait_for_log_contains_from_mark  ${mark}  Update failed  timeout=${60}
    check_log_contains_after_mark  ${SUL_DOWNLOADER_LOG}  Connecting to update source directly  mark=${mark}
