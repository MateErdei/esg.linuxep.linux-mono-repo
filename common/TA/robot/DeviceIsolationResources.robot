*** Settings ***
Library         Collections
Library         Process
Library         OperatingSystem
Library         String

Library   ${COMMON_TEST_LIBS}/ActionUtils.py
Library   ${COMMON_TEST_LIBS}/FakeMCS.py
Library   ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library   ${COMMON_TEST_LIBS}/LogUtils.py
Library   ${COMMON_TEST_LIBS}/OnFail.py
Library   ${COMMON_TEST_LIBS}/OSUtils.py
Library   ${COMMON_TEST_LIBS}/PolicyUtils.py
Library   ${COMMON_TEST_LIBS}/TemporaryDirectoryManager.py

# For can curl url
Library         ${COMMON_TEST_LIBS}/UpdateServer.py

Resource  ${COMMON_TEST_ROBOT}/GeneralUtilsResources.robot

*** Variables ***
${DEVICE_ISOLATION_BIN_PATH}            ${COMPONENT_ROOT_PATH}/bin/deviceisolation
${DEVICE_ISOLATION_LOG_PATH}            ${COMPONENT_ROOT_PATH}/log/deviceisolation.log
${PERSISTENT_STATE_FILE}                ${COMPONENT_ROOT_PATH}/var/persist-isolationEnabled
${DEVICE_ISOLATION_NFT_RULES_PATH}      ${COMPONENT_ROOT_PATH}/var/nft_rules.conf
${NTP_STATUS_XML}                       ${SOPHOS_INSTALL}/base/mcs/status/NTP_status.xml
${BASE_SDDS}                            ${TEST_INPUT_PATH}/base_sdds/
${NONSOPHOS_URL}                        www.google.com

*** Keywords ***
Install Base For Component Tests
    File Should Exist     ${BASE_SDDS}/install.sh
    Run Shell Process   bash -x ${BASE_SDDS}/install.sh --log-level DEBUG 2>/tmp/installer.log   OnError=Failed to Install Base   timeout=60s
    Run Keyword and Ignore Error   Run Shell Process    /opt/sophos-spl/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter

Install Device Isolation Directly from SDDS
    ${DEVICE_ISOLATION_SDDS} =   get_sspl_device_isolation_plugin_sdds
    File Should Exist   ${DEVICE_ISOLATION_SDDS}/install.sh
    File Should Exist   ${DEVICE_ISOLATION_SDDS}/files/plugins/deviceisolation/bin/deviceisolation
    File Should Exist   ${DEVICE_ISOLATION_SDDS}/files/plugins/deviceisolation/bin/nft
    ${result} =   Run Process  bash  -x  ${DEVICE_ISOLATION_SDDS}/install.sh   timeout=120s   stderr=STDOUT
    log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to install deviceisolation.\noutput: \n${result.stdout}"

Uninstall Base
    ${result} =   Run Process  bash  ${SOPHOS_INSTALL}/bin/uninstall.sh  --force   timeout=30s  stderr=STDOUT
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to uninstall base.\noutput: \n${result.stdout}"
    
Uninstall Device Isolation
    ${result} =   Run Process  bash ${SOPHOS_INSTALL}/plugins/deviceisolation/bin/uninstall.sh --force   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to uninstall Device Isolation.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"

Downgrade Device Isolation
    ${result} =   Run Process  bash ${SOPHOS_INSTALL}/plugins/deviceisolation/bin/uninstall.sh --downgrade   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to downgrade Device Isolation.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"

Check Device Isolation Executable Running
    ${result} =    Run Process  pgrep deviceisolation | wc -w  shell=true
    Should Be Equal As Integers    ${result.stdout}    ${1}       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Wait For Device Executable Running
    Wait Until Keyword Succeeds
    ...  14 secs
    ...  1 secs
    ...  Check Device Isolation Executable Running

Check Device Isolation Executable Not Running
    ${result} =    Run Process  pgrep  -a  deviceisolation
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    ${0}     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Wait For Device Executable Not Running
    Wait Until Keyword Succeeds
    ...  14 secs
    ...  1 secs
    ...  Check Device Isolation Executable Not Running

Stop Device Isolation
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl stop deviceisolation   OnError=failed to stop deviceisolation  timeout=35s

Start Device Isolation
    Run Shell Process  ${SOPHOS_INSTALL}/bin/wdctl start deviceisolation   OnError=failed to start deviceisolation

Restart Device Isolation
    ${di_mark} =    Get Device Isolation Log Mark
    Stop Device Isolation
    Wait For Log Contains From Mark    ${di_mark}    deviceisolation <> Plugin Finished

    ${di_mark} =    Get Device Isolation Log Mark
    Start Device Isolation
    Wait For Log Contains From Mark    ${di_mark}    Completed initialization of Device Isolation


List File In Test Dir
    ${filenames} =  List Directory  /opt/test/inputs/fake_management/
    Log    ${filenames}


Get Device Isolation Log Mark
    ${mark} =  mark_log_size  ${DEVICE_ISOLATION_LOG_PATH}
    [Return]  ${mark}

Device Isolation Suite Setup
    Install Base For Component Tests
    Install Device Isolation Directly from SDDS

Device Isolation Suite Teardown
    Uninstall Base

Device Isolation Test Setup
    Register on fail  dump log  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Register on fail  dump log  ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Register on fail  dump log  ${DEVICE_ISOLATION_LOG_PATH}

Device Isolation Test Teardown
	Run teardown functions
	Cleanup Temporary Folders
	Remove File    ${SOPHOS_INSTALL}/base/mcs/policy/NTP-24_policy.xml

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

Enable Device Isolation
    ${mark} =  Get Device Isolation Log Mark
    Send Enable Isolation Action  uuid=123
    Wait For Log Contains From Mark  ${mark}  Enabling Device Isolation
    Wait For Log Contains From Mark  ${mark}  Device is now isolated

Disable Device Isolation No Wait
    Send Disable Isolation Action    uuid=123

Disable Device Isolation
    ${mark} =  Get Device Isolation Log Mark
    Disable Device Isolation No Wait
    Wait For Log Contains From Mark  ${mark}  Disabling Device Isolation
    Wait For Log Contains From Mark  ${mark}  Device is no longer isolated

Send Device Isolation Policy
    [Arguments]  ${policyFile}
    ${srcFileName} =  Set Variable  ${ROBOT_SCRIPTS_PATH}/policies/${policyFile}
    send_policy  ${srcFileName}    NTP-24_policy.xml

Send Isolation Policy With CI Exclusions
    ${mark} =  Get Device Isolation Log Mark
    generate_isolation_policy_with_ci_exclusions    ${ROBOT_SCRIPTS_PATH}/policies/NTP-24_policy_with_exclusions.xml    ${ROBOT_SCRIPTS_PATH}/policies/NTP-24_policy_generated.xml
    Send Device Isolation Policy    NTP-24_policy_generated.xml
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

Send Isolation Policy With CI Exclusions And Extra IP Exclusion
    ${mark} =  Get Device Isolation Log Mark
    generate_isolation_policy_with_ci_exclusions    ${ROBOT_SCRIPTS_PATH}/policies/NTP-24_policy_with_exclusions_and_extra_ip_exclusion.xml    ${ROBOT_SCRIPTS_PATH}/policies/NTP-24_policy_generated.xml
    Send Device Isolation Policy    NTP-24_policy_generated.xml
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${mark}  Device Isolation policy applied

Add Exclusion To Isolation Policy Using URL And Port
    [Arguments]   ${exclusions}
    # This will work with both adding a single exclusion and multiple
    # The ${exclusions} can be a tuple - (url, port) or a list of tuples [(url1, port1), (url2, port2), ...]
    # Port can be None
    add url exclusions to policy
    ...    ${ROBOT_SCRIPTS_PATH}/policies/NTP-24_policy_with_exclusions.xml
    ...    ${ROBOT_SCRIPTS_PATH}/policies/NTP-24_policy_generated.xml
    ...    ${exclusions}
    Send Device Isolation Policy    NTP-24_policy_generated.xml

Add Exclusion To Isolation Policy Using IP and Port
    [Arguments]    ${exclusions}
    # The ${exclusions} needs to be a list of tuples [(url1, port1), (url2, port2), ...]
    # Port can be None
    generate isolation policy with exclusions
    ...    ${ROBOT_SCRIPTS_PATH}/policies/NTP-24_policy_with_exclusions.xml
    ...    ${ROBOT_SCRIPTS_PATH}/policies/NTP-24_policy_generated.xml
    ...    ${exclusions}
    Send Device Isolation Policy    NTP-24_policy_generated.xml

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
    ${contains_icmp}=  Evaluate   "ip protocol icmp accept" in """${result.stdout}"""
    ${contains_icmp_as_1}=  Evaluate   "ip protocol 1 accept" in """${result.stdout}"""
    IF    not (${contains_icmp} or ${contains_icmp_as_1})
          Fail    Could not find "ip protocol icmp accept" or "ip protocol 1 accept" in ruleset.
    END

Disable Device Isolation If Not Already
    ${is_disabled} =  Does File Contain Word  ${PERSISTENT_STATE_FILE}  0
    IF    ${is_disabled} is ${FALSE}
        ${mark} =  Get Device Isolation Log Mark
        Disable Device Isolation No Wait
        ${expected} =  Create List     Tried to disable isolation but it was already disabled in the first place     Device is no longer isolated
        Wait For Log Contains From Mark    ${mark}      ${expected}
    END