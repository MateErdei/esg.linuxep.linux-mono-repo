*** Settings ***
Library         Collections
Library         OperatingSystem

Resource        ${COMMON_TEST_ROBOT}/DeviceIsolationResources.robot
Library         ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library         ${TEST_INPUT_PATH}/fake_management/FakeManagement.py

Suite Setup     Device Isolation Suite Setup
Suite Teardown  Device Isolation Suite Teardown

Test Setup     Device Isolation Test Setup
Test Teardown  Device Isolation Test Teardown

*** Test Cases ***
Check Health Telemetry Is Written
    ${telemetry_json} =  Get Plugin Telemetry  deviceisolation
    Log  ${telemetry_json}
    ${telemetry_dict} =  Evaluate  json.loads('''${telemetry_json}''')  json

    Dictionary Should Contain Key  ${telemetry_dict}  health
    Should Be Equal As Integers  ${telemetry_dict["health"]}  0
