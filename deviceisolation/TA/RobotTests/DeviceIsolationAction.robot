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
    #Wait Until Keyword Succeeds    10s    1s    Check Rules Have Been Applied

    # Check we cannot access sophos.com because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com

    # Disable isolation
    Send Disable Isolation Action  uuid=1
    Wait For Log Contains From Mark  ${mark}  Disabling Device Isolation
    Wait For Log Contains From Mark  ${mark}  Device is no longer isolated
    Wait Until Keyword Succeeds    10s    1s    Can Curl Url    https://sophos.com

    # Network filtering rules should be empty
    ${result} =   Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    table    inet    sophos_device_isolation
    log  ${result.stdout}
    log  ${result.stderr}
    Should Be Equal As Strings  ${result.stdout}    ${EMPTY}
    Should Contain    ${result.stderr}    Error: No such file or directory
    Should Contain    ${result.stderr}    list table inet sophos_device_isolation
    Should Be Equal As Integers  ${result.rc}  ${1}   "nft list ruleset failed incorrectly with rc=${result.rc}"


Disable Device Isolation While already Disabled
    ${mark} =  Get Device Isolation Log Mark

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    # Isolate the endpoint
    Enable Device Isolation
    Wait Until Created  ${COMPONENT_ROOT_PATH}/var/nft_rules
    Log File    ${COMPONENT_ROOT_PATH}/var/nft_rules
    #Wait Until Keyword Succeeds    10s    1s    Check Rules Have Been Applied

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

    # Disable isolation again
    Send Disable Isolation Action  uuid=1
    Wait For Log Contains From Mark  ${mark}  Disabling Device Isolation
    Wait For Log Contains From Mark  ${mark}  Tried to disable isolation but it was not enabled in the first place
    Wait Until Keyword Succeeds    10s    1s    Can Curl Url    https://sophos.com

    # Network filtering rules should be empty
    ${result} =   Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    ruleset
    log    ${result.stdout}
    log    ${result.stderr}
    Should Be Equal As Strings  ${result.stdout}    ${EMPTY}


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
    #Wait Until Keyword Succeeds    10s    1s    Check Rules Have Been Applied

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


Device Isolation Allows Sophos Processes
    # LINUXDAR-8529: sudo --group does not seem to be available on all distros
    [Tags]    EXCLUDE_AMZLINUX2    EXCLUDE_AMZLINUX2023    EXCLUDE_CENTOS7    EXCLUDE_CENTOS8STREAM    EXCLUDE_CENTOS9STREAM    EXCLUDE_ORACLE79_X64    EXCLUDE_ORACLE87_X64    EXCLUDE_RHEL79    EXCLUDE_RHEL87    EXCLUDE_RHEL91    EXCLUDE_SLES12
    ${is_oracle} =  Does File Contain Word  /etc/os-release  Oracle Linux
    Pass Execution If  ${is_oracle}  LINUXDAR-8529 - exclude tags not working for Oracle 
    ${mark} =  Get Device Isolation Log Mark

    ${external_url} =    Set Variable    https://artifactory.sophos-ops.com
    Can Curl Url As Group    ${external_url}    group=sophos-spl-group

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    # Isolate the endpoint
    Enable Device Isolation
    Wait Until Created  ${COMPONENT_ROOT_PATH}/var/nft_rules
    ${nft_rules} =    Get File    ${COMPONENT_ROOT_PATH}/var/nft_rules
    #Wait Until Keyword Succeeds    10s    1s    Check Rules Have Been Applied
    ${sophos_gid} =    Get Gid From Groupname    sophos-spl-group
    Should Contain    ${nft_rules}    meta skgid ${sophos_gid} accept

    # Check we can access sophos.com as sophos group due to accept rule.
    Can Curl Url As Group    ${external_url}    group=sophos-spl-group
    # Check we cannot access sophos.com as non-sophos group because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: ${external_url}    Can Curl Url As Group    ${external_url}    group=nogroup

    # Disable isolation
    Send Disable Isolation Action  uuid=1
    Wait For Log Contains From Mark  ${mark}  Disabling Device Isolation
    Wait For Log Contains From Mark  ${mark}  Device is no longer isolated
    Wait Until Keyword Succeeds    10s    1s    Can Curl Url As Group    ${external_url}    group=nogroup

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
    # the output from this command gets truncated on some platforms
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

