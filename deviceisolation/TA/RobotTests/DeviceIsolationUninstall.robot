*** Settings ***
Resource        ${COMMON_TEST_ROBOT}/DeviceIsolationResources.robot

Suite Setup     Device Isolation Suite Setup
Suite Teardown  Device Isolation Suite Teardown

Test Setup     Device Isolation Test Setup
Test Teardown  Device Isolation Test Teardown


*** Test Cases ***

Device Isolation Network Rules Are Removed When Uninstalled
    ${mark} =  Get Device Isolation Log Mark

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    # Isolate the endpoint
    Enable Device Isolation

    # Check we cannot access sophos.com because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com

    # Uninstall Device Isolation and expect rules to be removed
    Uninstall Device Isolation

    # Once uninstalled the rules should be cleared and we should be able to access sophos.com
    Wait Until Keyword Succeeds    10s    1s    Can Curl Url    https://sophos.com

    # Network filtering rules should be empty
    ${DEVICE_ISOLATION_SDDS} =   get_sspl_device_isolation_plugin_sdds
    Run Process    chmod    +x    ${DEVICE_ISOLATION_SDDS}/files/plugins/deviceisolation/bin/nft
    ${result} =   Run Process    ${DEVICE_ISOLATION_SDDS}/files/plugins/deviceisolation/bin/nft    list    table    inet    sophos_device_isolation
    Should Be Equal As Strings  ${result.stdout}    ${EMPTY}
    Should Contain    ${result.stderr}    Error: No such file or directory
    Should Contain    ${result.stderr}    list table inet sophos_device_isolation
    Should Be Equal As Integers  ${result.rc}  ${1}   "nft list ruleset failed incorrectly with rc=${result.rc}"
