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