*** Settings ***
Resource        ${COMMON_TEST_ROBOT}/DeviceIsolationResources.robot

Library         ${COMMON_TEST_LIBS}/OSUtils.py
Library         ${COMMON_TEST_LIBS}/ActionUtils.py
Library         ${COMMON_TEST_LIBS}/FakeMCS.py
Library         ${COMMON_TEST_LIBS}/LogUtils.py

Suite Setup     Device Isolation Suite Setup
Suite Teardown  Device Isolation Suite Teardown

Test Setup     Device Isolation Test Setup
Test Teardown  Device Isolation Test Teardown


*** Test Cases ***

Device Isolation Applies Network Filtering Rules
    ${mark} =  Get Device Isolation Log Mark

    # Baseline - should be able to access sophos.com before isolation but not after.
    Can Curl Url    https://sophos.com
    Remove File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    Should Exist    ${COMPONENT_ROOT_PATH}/bin/nft

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    # Isolate the endpoint
    Enable Device Isolation
    Wait Until Created  ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    Log File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    #Wait Until Keyword Succeeds    10s    1s    Check Rules Have Been Applied

    # Check we cannot access sophos.com because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com

    # Disable isolation
    Disable Device Isolation
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
    Wait Until Created  ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    Log File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    #Wait Until Keyword Succeeds    10s    1s    Check Rules Have Been Applied

    # Disable isolation
    Disable Device Isolation
    Wait Until Keyword Succeeds    10s    1s    Can Curl Url    https://sophos.com

    # Network filtering rules should not contain our rules
    ${result} =   Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    table    inet    sophos_device_isolation
    log    ${result.stdout}
    log    ${result.stderr}
    Should Contain    ${result.stderr}    Error: No such file or directory
    Should Contain    ${result.stderr}    list table inet sophos_device_isolation
    Should Be Equal As Integers  ${result.rc}  ${1}   "nft list ruleset failed incorrectly with rc=${result.rc}"

    # Disable isolation again
    Send Disable Isolation Action  uuid=1
    Wait For Log Contains From Mark  ${mark}  Disabling Device Isolation
    Wait For Log Contains From Mark  ${mark}  Tried to disable isolation but it was already disabled in the first place
    Wait Until Keyword Succeeds    10s    1s    Can Curl Url    https://sophos.com

    # Network filtering rules should not contain our rules
    ${result} =   Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    table    inet    sophos_device_isolation
    log    ${result.stdout}
    log    ${result.stderr}
    Should Contain    ${result.stderr}    Error: No such file or directory
    Should Contain    ${result.stderr}    list table inet sophos_device_isolation
    Should Be Equal As Integers  ${result.rc}  ${1}   "nft list ruleset failed incorrectly with rc=${result.rc}"

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
    Wait Until Created  ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    Log File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    #Wait Until Keyword Succeeds    10s    1s    Check Rules Have Been Applied

    # Check we cannot access sophos.com because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com
    # Check we can access localhost
    Can Curl Url    http://localhost:8001

    # Disable isolation
    Disable Device Isolation
    Wait Until Keyword Succeeds    10s    1s    Can Curl Url    https://sophos.com
    Can Curl Url    http://localhost:8001

    # Network filtering rules should not contain our rules
    ${result} =   Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    table    inet    sophos_device_isolation
    log  ${result.stdout}
    log  ${result.stderr}
    Should Be Equal As Strings  ${result.stdout}    ${EMPTY}
    Should Contain    ${result.stderr}    Error: No such file or directory
    Should Contain    ${result.stderr}    list table inet sophos_device_isolation
    Should Be Equal As Integers  ${result.rc}  ${1}   "nft list ruleset failed incorrectly with rc=${result.rc}"

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
    Wait Until Created  ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    ${nft_rules} =    Get File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    #Wait Until Keyword Succeeds    10s    1s    Check Rules Have Been Applied
    ${sophos_gid} =    Get Gid From Groupname    sophos-spl-group
    Should Contain    ${nft_rules}    meta skgid ${sophos_gid} accept

    # Check we can access sophos.com as sophos group due to accept rule.
    Can Curl Url As Group    ${external_url}    group=sophos-spl-group
    # Check we cannot access sophos.com as non-sophos group because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: ${external_url}    Can Curl Url As Group    ${external_url}    group=nogroup

    # Disable isolation
    Disable Device Isolation
    Wait Until Keyword Succeeds    10s    1s    Can Curl Url As Group    ${external_url}    group=nogroup

    # Network filtering rules should be empty
    ${result} =   Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    ruleset
    log  ${result.stdout}
    log  ${result.stderr}
    Should Be Equal As Strings  ${result.stdout}    ${EMPTY}

Device Isolation Updates Network Filtering Rules With New Exclusion
    ${mark} =  Get Device Isolation Log Mark

    # Baseline - should be able to access sophos.com before isolation but not after.
    Can Curl Url    https://sophos.com
    Remove File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    Should Exist    ${COMPONENT_ROOT_PATH}/bin/nft

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    # Isolate the endpoint
    Enable Device Isolation
    Wait Until Created  ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    Log File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf

    # Check we cannot access sophos.com because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com

    ${mark} =  Get Device Isolation Log Mark
    # Send policy with new exclusion and check it gets added to ruleset
    Send Isolation Policy With CI Exclusions And Extra IP Exclusion
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    # Check Endpoint is still isolated and new ip exclusion was added
    Wait For Log Contains From Mark    ${mark}    Device is now isolated
    Log File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    ${nft_rules_file_content} =    Get File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    ${nft_rules_table} =    Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    table    inet    sophos_device_isolation
    Should Contain    ${nft_rules_file_content}    ip daddr 8.8.8.8 accept
    Should Be Equal As Integers    ${nft_rules_table.rc}    ${0}
    Should Contain    ${nft_rules_table.stdout}    ip daddr 8.8.8.8 accept

    # Check we cannot access sophos.com because the EP should still be isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com

    ${mark} =  Get Device Isolation Log Mark
    # Send policy with exclusions but not extra IP exclusion to check it gets removed from ruleset
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    # Check Endpoint is still isolated and new ip exclusion was removed
    Wait For Log Contains From Mark    ${mark}    Device is now isolated
    Log File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    ${nft_rules_file_content} =    Get File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    ${nft_rules_table} =    Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    table    inet    sophos_device_isolation
    Should Not Contain    ${nft_rules_file_content}    ip daddr 8.8.8.8 accept
    Should Be Equal As Integers    ${nft_rules_table.rc}    ${0}
    Should Not Contain    ${nft_rules_table.stdout}    ip daddr 8.8.8.8 accept

    # Check we cannot access sophos.com because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com

    Disable Device Isolation

Device Isolation Does not Enable Or Disable Isolation After Receiving Duplicate Policy
    ${mark} =  Get Device Isolation Log Mark

    # Baseline - should be able to access sophos.com before isolation but not after.
    Can Curl Url    https://sophos.com
    Remove File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    Should Exist    ${COMPONENT_ROOT_PATH}/bin/nft

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    # Isolate the endpoint
    Enable Device Isolation
    Wait Until Created  ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    Log File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf

    # Check we cannot access sophos.com because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com

    ${mark} =  Get Device Isolation Log Mark
    ${old_nft_rules_file} =    Get File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    ${old_nft_rules_table} =    Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    table    inet    sophos_device_isolation
    Should Be Equal As Integers    ${old_nft_rules_table.rc}    ${0}
    # Send Duplicate Policy
    Send Device Isolation Policy    NTP-24_policy_generated.xml
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    # Check Endpoint is still isolated and new ip exclusion was added
    Check Log Does Not Contain After Mark    ${DEVICE_ISOLATION_LOG_PATH}    Enabling Device Isolation    ${mark}
    Check Log Does Not Contain After Mark    ${DEVICE_ISOLATION_LOG_PATH}    Disabling Device Isolation    ${mark}
    Log File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    ${new_nft_rules_file} =    Get File    ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
    ${new_nft_rules_table} =    Run Process    ${COMPONENT_ROOT_PATH}/bin/nft    list    table    inet    sophos_device_isolation
    Should Be Equal As Strings    ${new_nft_rules_file}    ${old_nft_rules_file}
    Should Be Equal As Integers    ${new_nft_rules_table.rc}    ${0}
    Should Be Equal As Strings    ${old_nft_rules_table.stdout}    ${new_nft_rules_table.stdout}

    # Check we cannot access sophos.com because the EP should still be isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com

    Disable Device Isolation


Device Isolation Represents State Correctly When Isolation State Changes
    ${mark} =  Get Device Isolation Log Mark

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    Enable Device Isolation
    Wait For Log Contains From Mark  ${mark}  Enabling Device Isolation
    File Should Contain    ${NTP_STATUS_XML}    isolation self="false" admin="true"
    File Should Contain    ${PERSISTENT_STATE_FILE}    1

    Disable Device Isolation
    Wait For Log Contains From Mark  ${mark}  Disabling Device Isolation
    File Should Contain    ${NTP_STATUS_XML}    isolation self="false" admin="false"
    File Should Contain    ${PERSISTENT_STATE_FILE}    0


Failure To Remove Isolation Results in Isolated Status
    ${mark} =  Get Device Isolation Log Mark

    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

    Enable Device Isolation
    Wait For Log Contains From Mark  ${mark}  Enabling Device Isolation
    File Should Contain    ${NTP_STATUS_XML}    isolation self="false" admin="true"

    Stop Device Isolation
    Check Device Isolation Executable Not Running

    ${mgmt_mark} =    Mark Managementagent Log
    Disable Device Isolation No Wait
    Wait For Log Contains From Mark  ${mgmt_mark}  Process new action from mcsrouter: SHS_action

    File Should Contain    ${NTP_STATUS_XML}    isolation self="false" admin="true"