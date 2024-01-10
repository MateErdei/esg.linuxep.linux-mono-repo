*** Settings ***
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/OSUtils.py
Library     ${COMMON_TEST_LIBS}/ProcessUtils.py

Resource    ${COMMON_TEST_ROBOT}/DeviceIsolationResources.robot

Suite Setup     Device Isolation Suite Setup
Suite Teardown  Device Isolation Suite Teardown

Test Setup     Device Isolation Test Setup
Test Teardown  Device Isolation Test Teardown

*** Test Cases ***
Device Isolation Log Files Are Saved When Downgrading
	Register Cleanup  Install Device Isolation Directly from SDDS
    Downgrade Device Isolation

    # check that the log folder only contains the downgrade-backup directory
    ${log_dir} =  List Directory  ${SOPHOS_INSTALL}/plugins/deviceisolation/log
    Length Should Be  ${log_dir}  ${1}  log directory contains more than downgrade-backup
    Should Contain  ${log_dir}  downgrade-backup
    ${log_dir_files} =  List Files In Directory  ${SOPHOS_INSTALL}/plugins/deviceisolation/log
    Log  ${log_dir_files}
    Length Should Be  ${log_dir_files}  ${0}

    # check that the downgrade-backup directory contains the deviceisolation.log file
    ${backup_dir_files} =  List Files In Directory  ${SOPHOS_INSTALL}/plugins/deviceisolation/log/downgrade-backup
    Log  ${backup_dir_files}
    Length Should Be  ${backup_dir_files}  ${1}  downgrade-backup directory contains more than deviceisolation.log
    Should Contain  ${backup_dir_files}  deviceisolation.log


Device Isolation Plugin Installs With Version Ini File
    File Should Exist   ${SOPHOS_INSTALL}/plugins/deviceisolation/VERSION.ini
    VERSION Ini File Contains Proper Format For Product Name   ${SOPHOS_INSTALL}/plugins/deviceisolation/VERSION.ini   SPL-Device-Isolation-Plugin


Device Isolation Remains Isolated After SPL Restart
    [Tags]    EXCLUDE_CENTOS7    EXCLUDE_RHEL79
    ${di_mark} =    Mark Log Size    ${SOPHOS_INSTALL}/plugins/deviceisolation/log/deviceisolation.log
    # Send policy with exclusions
    Send Isolation Policy With CI Exclusions
    Log File    ${MCS_DIR}/policy/NTP-24_policy.xml
    Wait For Log Contains From Mark  ${di_mark}  Device Isolation policy applied

    # Isolate the endpoint
    Enable Device Isolation
    Wait Until Created  ${DEVICE_ISOLATION_NFT_RULES_PATH}
    Log File    ${DEVICE_ISOLATION_NFT_RULES_PATH}
    Wait Until Keyword Succeeds    10s    1s    Check Rules Have Been Applied

    # Check we cannot access sophos.com because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com

    ${di_mark} =    Mark Log Size    ${SOPHOS_INSTALL}/plugins/deviceisolation/log/deviceisolation.log
    Run Process    systemctl    restart    sophos-spl

    Wait For Log Contains From Mark    ${di_mark}    Completed initialization of Device Isolation
    Wait Until Keyword Succeeds    10s    1s    Check Rules Have Been Applied

    # Check we cannot access sophos.com because the EP is isolated.
    Run Keyword And Expect Error    cannot reach url: https://sophos.com    Can Curl Url    https://sophos.com


Diagnose Tool Gathers nft_rules File
    # Isolate the endpoint
    Enable Device Isolation
    Wait Until Created  ${DEVICE_ISOLATION_NFT_RULES_PATH}
    Log File    ${DEVICE_ISOLATION_NFT_RULES_PATH}
    Disable Device Isolation

    ${TarTempDir} =  Add Temporary Directory  tarTempdir
    Run Process  chmod  700  ${TarTempDir}

    ${result} =   Run Process   ${SOPHOS_INSTALL}/bin/sophos_diagnose    ${TarTempDir}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  0

    ${Files} =  List Files In Directory  ${TarTempDir}
    ${LengthOfFiles} =  Get Length  ${Files}
    should Be Equal As Numbers  1  ${LengthOfFiles}

    ${TarContents} =  Query Tarfile For Contents  ${TarTempDir}/${Files[0]}
    Log  ${TarContents}
    Should Contain 	${TarContents}  PluginFiles/deviceisolation/var/nft_rules.conf



Watchdog Restarts Device Isolation
    Check Device Isolation Executable Running

    ${PID} =  ProcessUtils.wait for pid  ${DEVICE_ISOLATION_BIN_PATH}
    Kill Process    ${PID}    9

    Wait For Device Executable Not Running
    Register Cleanup     Start Device Isolation
    Wait For Device Executable Running