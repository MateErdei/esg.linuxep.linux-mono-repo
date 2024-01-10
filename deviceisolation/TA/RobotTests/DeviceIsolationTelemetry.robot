*** Settings ***
Library         Collections
Library         OperatingSystem

Resource        ${COMMON_TEST_ROBOT}/DeviceIsolationResources.robot
Library         ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library         ${COMMON_TEST_LIBS}/FakeManagement.py

Suite Setup     Device Isolation Suite Setup
Suite Teardown  Device Isolation Suite Teardown

Test Setup     Device Isolation Test Setup
Test Teardown  Device Isolation Test Teardown

*** Test Cases ***
Check Health Telemetry Is Written
    Restart Device Isolation
    ${telemetry_json} =  Get Plugin Telemetry  deviceisolation
    Log  ${telemetry_json}
    ${telemetry_dict} =  Evaluate  json.loads('''${telemetry_json}''')  json

    Dictionary Should Contain Key  ${telemetry_dict}  health
    Should Be Equal As Integers  ${telemetry_dict["health"]}  0

Check Is Isolation Active Telemetry Is Written
    Restart Device Isolation
    ${telemetry_json} =  Get Plugin Telemetry  deviceisolation
    Log  ${telemetry_json}
    ${telemetry_dict} =  Evaluate  json.loads('''${telemetry_json}''')  json

    Dictionary Should Contain Key  ${telemetry_dict}  currently-active
    Should Be Equal As Strings    ${telemetry_dict["currently-active"]}    False

Check Is Isolation Active Telemetry Is Updated When Isolate Action Received
    Restart Device Isolation
    Should Exist   ${COMPONENT_ROOT_PATH}/bin/nft
    Remove File    ${DEVICE_ISOLATION_NFT_RULES_PATH}
    ${di_mark} =    Mark Log Size    ${DEVICE_ISOLATION_LOG_PATH}

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${di_mark}  Device Isolation policy applied

    Enable Device Isolation
    Wait For Log Contains From Mark    ${di_mark}    Finished processing action

    ${telemetry_json} =  Get Plugin Telemetry  deviceisolation
    Log  ${telemetry_json}
    ${telemetry_dict} =  Evaluate  json.loads('''${telemetry_json}''')  json

    Dictionary Should Contain Key  ${telemetry_dict}  currently-active
    Should Be Equal As Strings    ${telemetry_dict["currently-active"]}    True

    ${di_mark} =    Mark Log Size    ${DEVICE_ISOLATION_LOG_PATH}
    Send Disable Isolation Action    uuid=1
    Wait For Log Contains From Mark  ${di_mark}  Disabling Device Isolation

Check Is Isolation Active Telemetry Is Updated When Disable Isolation Action Received After Enable Isolate Action
    Restart Device Isolation
    Should Exist   ${COMPONENT_ROOT_PATH}/bin/nft
    Remove File    ${DEVICE_ISOLATION_NFT_RULES_PATH}
    ${di_mark} =    Mark Log Size    ${DEVICE_ISOLATION_LOG_PATH}

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${di_mark}  Device Isolation policy applied

    Enable Device Isolation
    Wait For Log Contains From Mark  ${di_mark}  Enabling Device Isolation
    Wait For Log Contains From Mark    ${di_mark}    Finished processing action

    ${di_mark} =    Mark Log Size    ${DEVICE_ISOLATION_LOG_PATH}
    Send Disable Isolation Action    uuid=1
    Wait For Log Contains From Mark    ${di_mark}    Finished processing action

    ${telemetry_json} =  Get Plugin Telemetry  deviceisolation
    Log  ${telemetry_json}
    ${telemetry_dict} =  Evaluate  json.loads('''${telemetry_json}''')  json

    Dictionary Should Contain Key  ${telemetry_dict}  currently-active
    Should Be Equal As Strings    ${telemetry_dict["currently-active"]}    False

Check Was Isolation Activated In Last 24 Hours Telemetry Is Written
    Restart Device Isolation
    ${telemetry_json} =  Get Plugin Telemetry  deviceisolation
    Log  ${telemetry_json}
    ${telemetry_dict} =  Evaluate  json.loads('''${telemetry_json}''')  json

    Dictionary Should Contain Key  ${telemetry_dict}  activated-last-24-hours
    Should Be Equal As Strings    ${telemetry_dict["activated-last-24-hours"]}    False

Check Was Isolation Activated In Last 24 Hours Is Updated When Isolate Action Received
    Restart Device Isolation
    Should Exist   ${COMPONENT_ROOT_PATH}/bin/nft
    Remove File    ${DEVICE_ISOLATION_NFT_RULES_PATH}
    ${di_mark} =    Mark Log Size    ${DEVICE_ISOLATION_LOG_PATH}

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${di_mark}  Device Isolation policy applied

    Enable Device Isolation
    Wait For Log Contains From Mark    ${di_mark}    Finished processing action

    ${telemetry_json} =  Get Plugin Telemetry  deviceisolation
    Log  ${telemetry_json}
    ${telemetry_dict} =  Evaluate  json.loads('''${telemetry_json}''')  json

    Dictionary Should Contain Key  ${telemetry_dict}  activated-last-24-hours
    Should Be Equal As Strings    ${telemetry_dict["activated-last-24-hours"]}    True

    ${di_mark} =    Mark Log Size    ${DEVICE_ISOLATION_LOG_PATH}
    Send Disable Isolation Action    uuid=1
    Wait For Log Contains From Mark  ${di_mark}  Disabling Device Isolation