*** Settings ***
Test Teardown  Test Teardown
Library    Process
Library    ${LIBS_DIRECTORY}/LogUtils.py

Resource    ../upgrade_product/UpgradeResources.robot
Resource    SulDownloaderResources.robot

Default Tags  SULDOWNLOADER

*** Test Cases ***

Give invalid update config to Suldownloader running in sdds3 mode
    Setup Install SDDS3 Base
    Create File    ${UPDATE_CONFIG}  garbage

    Run Shell Process  systemctl start sophos-spl-update      failed to start suldownloader
    Wait Until Keyword Succeeds
    ...    10s
    ...    1s
    ...    Check Marked Sul Log Contains   Generating the report file
    Check Marked Sul Log Contains   Update failed, with code: 121
    Check Marked Sul Log Contains  Failed to process json message with error: INVALID_ARGUMENT:Unexpected token
    Log File  ${UPDATE_CONFIG}


SUS Fault Injection Server Down
    Suite Setup Without Ostia
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
        ...  Update failed, with code: 112
