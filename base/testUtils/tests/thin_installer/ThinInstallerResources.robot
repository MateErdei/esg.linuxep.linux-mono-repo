*** Settings ***

Resource  ThinInstallerResources.robot

*** Variables ***
${EtcGroupFilePath}  /etc/group
${EtcGroupFileBackupPath}  /etc/group.bak
${CUSTOM_DIR_BASE} =  /CustomPath
${CUSTOM_TEMP_UNPACK_DIR} =  /tmp/temporary-unpack-dir

*** Keywords ***

Setup Thininstaller Test
    Require Uninstalled
    Set Environment Variable  CORRUPTINSTALL  no
    Get Thininstaller
    Create Default Credentials File
    Build Default Creds Thininstaller From Sections

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
    Remove Directory  ${CUSTOM_DIR_BASE}  recursive=True
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

Setup For Test With Warehouse Containing Product
    Create Directory    ${tmpdir}
    Setup Warehouse For Base
    Require Uninstalled

Regenerate HTTPS Certificates
    Run Process    make    clean    cwd=${SUPPORT_FILES}/https/
    Run Process    make    all    cwd=${SUPPORT_FILES}/https/

Ostia Test Setup
    Require Uninstalled
    Set Environment Variable  CORRUPTINSTALL  no

Ostia Test Teardown
    [Arguments]  ${UninstallAudit}=True
    Run Keyword If Test Failed    Dump Cloud Server Log
    General Test Teardown
    Run Keyword If Test Failed    Dump Thininstaller Log
    Run Keyword If Test Failed    Log Status Of Sophos Spl
    Stop Local Cloud Server
    Cleanup Local Warehouse And Thininstaller
    Require Uninstalled
    Cleanup Temporary Folders
    Require Uninstalled

Ostia Suite Setup
    Regenerate HTTPS Certificates
    Regenerate Certificates
    Set Local CA Environment Variable
    Setup Ostia Warehouse Environment

Ostia Suite Teardown
    Teardown Ostia Warehouse Environment
    Run Process    make    clean    cwd=${SUPPORT_FILES}/https/

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