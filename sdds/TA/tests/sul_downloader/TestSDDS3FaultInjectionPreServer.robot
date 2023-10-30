*** Settings ***
Library    Process
Library    ${COMMON_TEST_LIBS}/LogUtils.py

Resource    ${COMMON_TEST_ROBOT}/GeneralUtilsResources.robot
Resource    ${COMMON_TEST_ROBOT}/SulDownloaderResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Test Teardown  Upgrade Resources Test Teardown

Force Tags  SULDOWNLOADER  TAP_PARALLEL1

*** Test Cases ***

Give invalid update config to Suldownloader running in sdds3 mode
    Setup Install SDDS3 Base
    Create File    ${UPDATE_CONFIG}  garbage

    ${mark} =  mark_log_size  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Run Shell Process  systemctl start sophos-spl-update      failed to start suldownloader
    wait_for_log_contains_from_mark  ${mark}  Generating the report file  timeout=${10}
    wait_for_log_contains_from_mark  ${mark}   Update failed, with code: 121  timeout=${10}
    wait_for_log_contains_from_mark  ${mark}   Failed to process json message with error: INVALID_ARGUMENT  timeout=${10}
    Log File  ${UPDATE_CONFIG}

SUS Fault Injection Server Down
    Upgrade Resources Suite Setup
    Start Local Cloud Server    --initial-alc-policy  ${SUPPORT_FILES}/CentralXml/ALC_FixedVersionPolicySDDS3.xml
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
    ...   10 secs
    ...   1 secs
    ...   Check Suldownloader Log Contains In Order
        ...  Failed to connect to repository: SUS request failed with error: Couldn't connect to server
        ...  Update failed, with code: 107