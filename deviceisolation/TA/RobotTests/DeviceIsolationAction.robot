*** Settings ***
Resource        ${COMMON_TEST_ROBOT}/DeviceIsolationResources.robot

Library         ${COMMON_TEST_LIBS}/OSUtils.py
Library         ${COMMON_TEST_LIBS}/ActionUtils.py
Library         ${COMMON_TEST_LIBS}/FakeMCS.py
Library         ${COMMON_TEST_LIBS}/LogUtils.py
Library         ${COMMON_TEST_LIBS}/PolicyUtils.py

# For can curl url
Library         ${COMMON_TEST_LIBS}/UpdateServer.py

Suite Setup     Device Isolation Suite Setup
Suite Teardown  Device Isolation Suite Teardown

Test Setup     Device Isolation Test Setup
Test Teardown  Device Isolation Test Teardown

Force Tags    EXCLUDE_CENTOS7    EXCLUDE_RHEL79

*** Test Cases ***

Device Isolation Applies Network Filtering Rules
    ${mark} =  Get Device Isolation Log Mark

    # Baseline - should be able to access sophos.com before isolation but not after.
    Can Curl Url    https://sophos.com
    Remove File    ${COMPONENT_ROOT_PATH}/var/nft_rules
    Should Exist    ${COMPONENT_ROOT_PATH}/bin/nft

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    # Isolate the endpoint
    Enable Device Isolation
    Wait Until Created  ${COMPONENT_ROOT_PATH}/var/nft_rules
    Log File    ${COMPONENT_ROOT_PATH}/var/nft_rules
    Wait Until Keyword Succeeds    10s    1s    Check Rules Have Been Applied

    # Check we cannot access sophos.com because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com

    # Disable isolation
    Send Disable Isolation Action  uuid=1
    Wait For Log Contains From Mark  ${mark}  Disabling Device Isolation
    Wait For Log Contains From Mark  ${mark}  Device is no longer isolated
    Wait Until Keyword Succeeds    10s    1s    Can Curl Url    https://sophos.com

    # Network filtering rules should be empty
    ${result} =   Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    ruleset
    log  ${result.stdout}
    log  ${result.stderr}
    Should Be Equal As Strings  ${result.stdout}    ${EMPTY}
    Should Be Equal As Strings  ${result.stderr}    ${EMPTY}
    Should Be Equal As Integers  ${result.rc}  ${0}   "nft list ruleset failed with rc=${result.rc}"


Device Isolation Allows Localhost
    ${mark} =  Get Device Isolation Log Mark

    # Baseline - should be able to access sophos.com before isolation but not after.
    Can Curl Url    https://sophos.com

    ${simple_server} =	Start Process	python3    -m    http.server    8001    -d  ${ROBOT_SCRIPTS_PATH}/actions/misc
    Register Cleanup   Terminate Process	${simple_server}
    Wait Until Keyword Succeeds    10s    1s    Can Curl Url    http://localhost:8001

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    # Isolate the endpoint
    Enable Device Isolation
    Wait Until Created  ${COMPONENT_ROOT_PATH}/var/nft_rules
    Log File    ${COMPONENT_ROOT_PATH}/var/nft_rules
    Wait Until Keyword Succeeds    10s    1s    Check Rules Have Been Applied

    # Check we cannot access sophos.com because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com
    # Check we can access localhost
    Can Curl Url    http://localhost:8001

    # Disable isolation
    Send Disable Isolation Action  uuid=1
    Wait For Log Contains From Mark  ${mark}  Disabling Device Isolation
    Wait For Log Contains From Mark  ${mark}  Device is no longer isolated
    Wait Until Keyword Succeeds    10s    1s    Can Curl Url    https://sophos.com
    Can Curl Url    http://localhost:8001

    # Network filtering rules should be empty
    ${result} =   Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    ruleset
    log  ${result.stdout}
    log  ${result.stderr}
    Should Be Equal As Strings  ${result.stdout}    ${EMPTY}


*** Keywords ***

Send Enable Isolation Action
    [Arguments]  ${uuid}
    Send Isolation Action  enable_isolation.xml  ${uuid}

Send Disable Isolation Action
    [Arguments]  ${uuid}
    Send Isolation Action  disable_isolation.xml  ${uuid}

Send Isolation Action
    [Arguments]  ${filename}  ${uuid}
    ${creation_time_and_ttl} =  get_valid_creation_time_and_ttl
    ${actionFileName} =    Set Variable    ${SOPHOS_INSTALL}/base/mcs/action/SHS_action_${creation_time_and_ttl}.xml
    ${srcFileName} =  Set Variable  ${ROBOT_SCRIPTS_PATH}/actions/${filename}
    send_action  ${srcFileName}  ${actionFileName}  UUID=${uuid}

Send Device Isolation Policy
    [Arguments]  ${policyFile}
    ${srcFileName} =  Set Variable  ${ROBOT_SCRIPTS_PATH}/policies/${policyFile}
    send_policy  ${srcFileName}    NTP-24_policy.xml

Send Isolation Policy With CI Exclusions
    generate_isolation_policy_with_ci_exclusions    ${ROBOT_SCRIPTS_PATH}/policies/NTP-24_policy_with_exclusions.xml    ${ROBOT_SCRIPTS_PATH}/policies/NTP-24_policy_generated.xml
    Send Device Isolation Policy    NTP-24_policy_generated.xml

Enable Device Isolation
    ${mark} =  Get Device Isolation Log Mark
    Send Enable Isolation Action  uuid=123
    Wait For Log Contains From Mark  ${mark}  Enabling Device Isolation
    Wait For Log Contains From Mark  ${mark}  Device is now isolated

Check Rules Have Been Applied
    ${result} =   Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    ruleset
    log  ${result.stdout}
    log  ${result.stderr}

    @{rules} =    Create List
        ...    tcp dport 443 accept
        ...    udp dport 443 accept
        ...    ip saddr 192.168.1.1 tcp dport 22 accept
        ...    ip saddr 192.168.1.1 udp dport 22 accept
        ...    tcp dport 53 accept
        ...    udp dport 53 accept
        ...    ip daddr 192.168.1.9 accept
        ...    ip daddr 192.168.1.1 tcp sport 22 accept
        ...    ip daddr 192.168.1.1 udp sport 22 accept

    FOR  ${rule}  IN  @{rules}
       Should Contain      ${result.stdout}    ${rule}
    END

    # On some platforms "icmp" is converted to "1" and "1" is converted to "icmp" so check one of them exists.
    ${conatins_icmp}=  Evaluate   "ip protocol icmp accept" in """${result.stdout}"""
    ${conatins_icmp_as_1}=  Evaluate   "ip protocol 1 accept" in """${result.stdout}"""
    IF    not (${conatins_icmp} or ${conatins_icmp_as_1})
          Fail    Could not find "ip protocol icmp accept" or "ip protocol 1 accept" in ruleset.
    END

