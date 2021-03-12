*** Variables ***
${EtcGroupFilePath}  /etc/group
${EtcGroupFileBackupPath}  /etc/group.bak

*** Keywords ***
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