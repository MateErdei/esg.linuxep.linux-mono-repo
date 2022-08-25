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