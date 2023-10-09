*** Settings ***

Resource  ../update/SDDS3Resources.robot

*** Variables ***
${EtcGroupFilePath}  /etc/group
${EtcGroupFileBackupPath}  /etc/group.bak
${CUSTOM_DIR_BASE} =  /CustomPath
${CUSTOM_TEMP_UNPACK_DIR} =  /tmp/temporary-unpack-dir

*** Keywords ***
Setup Update Tests
    Regenerate HTTPS Certificates
    Copy File   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem    ${SUPPORT_FILES}/https/ca/root-ca.crt
    Install System Ca Cert  ${SUPPORT_FILES}/https/ca/root-ca.crt
    Regenerate Certificates
    Set Local CA Environment Variable


Setup sdds3 Update Tests
    Set Suite Variable    ${GL_handle}    ${EMPTY}
    Generate Local Ssl Certs If They Dont Exist
    Install Local SSL Server Cert To System
    Generate Fake sdds3 warehouse
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Set Local CA Environment Variable

Cleanup sdds3 Update Tests
    Stop Local SDDS3 Server
    Clean up fake warehouse

sdds3 suite setup with fakewarehouse with real base
    Generate Local Ssl Certs If They Dont Exist
    Install Local SSL Server Cert To System
    Generate Warehouse From Local Base Input
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Set Local CA Environment Variable

sdds3 suite fake warehouse Teardown
    Clean up fake warehouse
    Stop Local SDDS3 Server

Setup base Install
    Require Installed
    Create File    ${SOPHOS_INSTALL}/base/mcs/certs/ca_env_override_flag
    Create File    ${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json
    Remove File    ${SOPHOS_INSTALL}/base/VERSION.ini.0
    ${result1} =   Run Process   cp ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base/VERSION.ini ${SOPHOS_INSTALL}/base/VERSION.ini.0  shell=true

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

Regenerate HTTPS Certificates
    Run Process    make    clean    cwd=${SUPPORT_FILES}/https/
    Run Process    make    all    cwd=${SUPPORT_FILES}/https/

Create Initial Installation
    Require Fresh Install
    Set Local CA Environment Variable

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