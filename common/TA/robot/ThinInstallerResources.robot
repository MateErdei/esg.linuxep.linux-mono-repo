*** Settings ***
Library    OperatingSystem

Resource    McsRouterResources.robot
Resource    SDDS3Resources.robot

Library     ${COMMON_TEST_LIBS}/OnFail.py
Library     ${COMMON_TEST_LIBS}/OSUtils.py
Library     ${COMMON_TEST_LIBS}/ThinInstallerUtils.py

*** Variables ***
${EtcGroupFilePath}  /etc/group
${EtcGroupFileBackupPath}  /etc/group.bak
${CUSTOM_DIR_BASE} =  /CustomPath
${CUSTOM_TEMP_UNPACK_DIR} =  /tmp/temporary-unpack-dir
${CUSTOM_THININSTALLER_REPORT_LOC} =    ${CUSTOM_TEMP_UNPACK_DIR}/thininstaller_report.ini
${CUSTOM_REGISTRATION_COMMS_CHECK_LOC} =    ${CUSTOM_TEMP_UNPACK_DIR}/registration_comms_check.ini
${CUSTOM_SUS_COMMS_CHECK_LOC} =    ${CUSTOM_TEMP_UNPACK_DIR}/sus_comms_check.ini
${CUSTOM_CDN_COMMS_CHECK_LOC} =    ${CUSTOM_TEMP_UNPACK_DIR}/cdn_comms_check.ini

*** Keywords ***
Setup Update Tests
    Setup_MCS_Cert_Override


Setup sdds3 Update Tests
    Set Suite Variable    ${GL_handle}    ${EMPTY}
    Generate Fake sdds3 warehouse
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup_MCS_Cert_Override

Cleanup sdds3 Update Tests
    Stop Local SDDS3 Server
    Clean up fake warehouse

sdds3 suite setup with fakewarehouse with real base
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Setup_MCS_Cert_Override

SDDS3 Suite Fake Warehouse Teardown
    Clean up fake warehouse
    Stop Local SDDS3 Server

Setup base Install
    Require Installed
    Create File    ${SOPHOS_INSTALL}/base/mcs/certs/ca_env_override_flag
    Create File    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json
    Remove File    ${SOPHOS_INSTALL}/base/VERSION.ini.0

    ${usingSystemProductTestInput}=    Directory Exists    ${SYSTEMPRODUCT_TEST_INPUT}
    IF    ${usingSystemProductTestInput}
        ${result} =   Run Process   cp ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base/VERSION.ini ${SOPHOS_INSTALL}/base/VERSION.ini.0  shell=true
    ELSE
        ${result} =   Run Process   cp ${TEST_INPUT_PATH}/base_sdds/VERSION.ini ${SOPHOS_INSTALL}/base/VERSION.ini.0  shell=true
    END

    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers   ${result.rc}  ${0}

Setup Thininstaller Test
    Start Local Cloud Server    --initial-alc-policy    ${SUPPORT_FILES}/CentralXml/ALC_policy/ALC_policy_base_only.xml
    Setup Thininstaller Test Without Local Cloud Server

Setup Thininstaller Test Without Local Cloud Server
    Require Uninstalled
    Get Thininstaller
    Create Default Credentials File
    Build Default Creds Thininstaller From Sections

Teardown With Temporary Directory Clean
    Thininstaller Test Teardown
    Remove Directory   ${tmpdir}  recursive=True

Teardown With Temporary Directory Clean And Stopping Message Relays
    Teardown With Temporary Directory Clean
    Stop Proxy If Running

Thininstaller Test Teardown
    OnFail.run_teardown_functions
    Run Keyword If Test Failed   dump_df
    General Test Teardown
    Stop Update Server
    Stop Proxy Servers
    Run Keyword If Test Failed   Dump Cloud Server Log
    Stop Local Cloud Server
    Cleanup Local Cloud Server Logs
    Teardown Reset Original Path
    Run Keyword If Test Failed    Dump Thininstaller Log
    Remove Thininstaller Log
    Cleanup Files
    Require Uninstalled
    Remove Environment Variable  SOPHOS_INSTALL
    Remove Directory  ${CUSTOM_TEMP_UNPACK_DIR}  recursive=True
    Remove Environment Variable  INSTALL_OPTIONS_FILE
    Cleanup Temporary Folders

Check Proxy Log Contains
    [Arguments]  ${pattern}  ${fail_message}
    ${ret} =  Grep File  ${PROXY_LOG}  ${pattern}
    Should Contain  ${ret}  ${pattern}  ${fail_message}

Check MCS Config Contains
    [Arguments]  ${pattern}  ${fail_message}
    ${ret} =  Grep File  ${MCS_CONFIG_FILE}  ${pattern}
    Should Contain  ${ret}  ${pattern}  ${fail_message}


Create Initial Installation
    Require Fresh Install
    Setup_MCS_Cert_Override

Check Root Directory Permissions Are Not Changed
    ${result}=  Run Process  stat  -c  "%A"  /
    ${ExpectedPerms}=  Set Variable  "dr[w-]xr-xr-x"
    Should Match Regexp  ${result.stdout}  ${ExpectedPerms}

Setup Group File With Large Group Creation
    Copy File  ${EtcGroupFilePath}  ${EtcGroupFileBackupPath}
    ${LongLine} =  Get File  ${SUPPORT_FILES}/misc/325CharEtcGroupLine
    ${OriginalFileContent} =   Get File   ${EtcGroupFilePath}
    Create File  ${EtcGroupFilePath}  ${LongLine}
    Append To File  ${EtcGroupFilePath}  ${OriginalFileContent}
    Log File  ${EtcGroupFilePath}

Teardown Group File With Large Group Creation
    Move File  ${EtcGroupFileBackupPath}  ${EtcGroupFilePath}
    ${content} =  Get File  ${EtcGroupFilePath}
    ${LongLine} =  Get File  ${SUPPORT_FILES}/misc/325CharEtcGroupLine
    Should Not Contain  ${content}  ${LongLine}

Find IP Address With Distance
    [Arguments]  ${dist}
    ${result} =  Run Process  ip  addr
    ${ipaddresses} =  Get Regexp Matches  ${result.stdout}  inet (10[^/]*)  1
    ${head} =  Get Regexp Matches  ${ipaddresses[0]}  (10\.[^\.]*\.[^\.]*\.)[^\.]*  1
    ${tail} =  Get Regexp Matches  ${ipaddresses[0]}  10\.[^\.]*\.[^\.]*\.([^\.]*)  1
    ${tail_xored} =  Evaluate  ${tail[0]} ^ (2 ** ${dist})
    [return]  ${head[0]}${tail_xored}

Check Thininstaller Log Does Not Contain Error
    ${log} =  ThinInstallerUtils.get_thin_installer_log_path
    Mark Expected Error In Log    ${log}    ERROR Refusing to send telemetry with empty tenant ID
    Check Thininstaller Log Does Not Contain  ERROR
