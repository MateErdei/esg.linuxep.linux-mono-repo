*** Settings ***

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/DiagnoseUtils.py

Library     ${LIBS_DIRECTORY}/OSUtils.py

Library     OperatingSystem

Resource  ../GeneralTeardownResource.robot

*** Variables ***

${TAR_FILE_DIRECTORY} =  /tmp/TestOutputDirectory
${UNPACK_DIRECTORY} =  /tmp/DiagnoseOutput

*** Keywords ***

Display All Extracted Files
    ${result} =    Run Process  find   /tmp/DiagnoseOutput
    Log  ${result.stdout}

Teardown
    General Test Teardown
    Run Keyword If Test Failed    LogUtils.Dump Log    /tmp/diagnose.log
    Run Keyword If Test Failed    Display All Extracted Files
    Remove Dir If Exists   ${TAR_FILE_DIRECTORY}
    Remove Dir If Exists   ${UNPACK_DIRECTORY}
    Run Process  umount  /tmp/up
    Remove Dir If Exists   /tmp/5mbarea
    Remove Dir If Exists   /tmp/up



Check Diagnose Base Output
    ${Files} =  List Files In Directory  /tmp/DiagnoseOutput/BaseFiles
    ${GeneratedComponentFiles} =  List Files In Directory   /tmp/DiagnoseOutput/BaseFiles/ServerProtectionLinux-Base

    # Base files
    Should Contain  ${Files}    ALC_status.xml
    Should Contain  ${Files}    DiagnosePaths.conf
    Should Contain  ${Files}    logger.conf
    Should Contain  ${Files}    machine_id.txt
    Should Contain  ${Files}    mcs_envelope.log
    Should Contain  ${Files}    mcsrouter.log
    Should Contain  ${Files}    sophos_managementagent.log
    Should Contain  ${Files}    updatescheduler.log
    Should Contain  ${Files}    watchdog.log
    Should Contain  ${Files}    wdctl.log
    Should Contain  ${Files}    managementagent.json
    Should Contain  ${Files}    updatescheduler.json
    Should Contain  ${Files}    mcsrouter.json
    Should Contain  ${Files}    VERSION.ini
    Should Contain  ${Files}    diagnose.log

    Should Contain  ${GeneratedComponentFiles}  addedFiles_manifest.dat
    Should Contain  ${GeneratedComponentFiles}  changedFiles_manifest.dat
    Should Contain  ${GeneratedComponentFiles}  removedFiles_manifest.dat


Check Diagnose Output For Plugin logs
    ${Files} =  List Files In Directory  /tmp/DiagnoseOutput/PluginFiles
    Should Contain  ${Files}    AuditPlugin.log
    Should Contain  ${Files}    EventProcessor.log

    # Check the plugin registry files
    ${Files} =  List Files In Directory  /tmp/DiagnoseOutput/BaseFiles
    Should Contain  ${Files}    AuditPlugin.json
    Should Contain  ${Files}    EventProcessor.json

Check Diagnose Output For Additional MDR Plugin File
    [Documentation]  Requires 'Mimic MDR Files' to be executed first
    ${MDRLogFiles} =  List Files In Directory  /tmp/DiagnoseOutput/PluginFiles/mtr/dbos/data/logs
    ${MDRDataFiles} =  List Files In Directory  /tmp/DiagnoseOutput/BaseFiles
    ${MTRVersionFile} =  List Files In Directory  /tmp/DiagnoseOutput/PluginFiles/mtr
    ${MTRDBOSVersionFile} =  List Files In Directory  /tmp/DiagnoseOutput/PluginFiles/mtr/dbos/data/
    Should Contain  ${MDRLogFiles}    some.log.file
    Should Contain  ${MDRDataFiles}    osquery.flags
    Should Contain  ${MTRVersionFile}    VERSION.ini
    Should Contain  ${MTRDBOSVersionFile}    VERSION.ini


Check Diagnose Output For System Command Files
    ${Files} =  List Files In Directory  /tmp/DiagnoseOutput/SystemFiles
    Should Contain  ${Files}    df
    Should Contain  ${Files}    top
    Should Contain  ${Files}    dstat
    Should Contain  ${Files}    iostat
    Should Contain  ${Files}    hostnamectl
    Should Contain  ${Files}    uname
    Should Contain  ${Files}    lscpu
    Should Contain  ${Files}    lshw
    Should Contain  ${Files}    systemd
    Should Contain  ${Files}    usr-systemd
    Should Contain  ${Files}    list-unit-files
    Should Contain  ${Files}    auditctl
    Should Contain  ${Files}    systemctl-status-auditd
    Should Contain  ${Files}    plugins.d
    Should Contain  ${Files}    journalctl-sophos-spl
    Should Contain  ${Files}    journalctl-auditd
    Should Contain  ${Files}    journalctl_TRANSPORT=audit
    Should Contain  ${Files}    journalctl-yesterday
    Should Contain  ${Files}    ps
    Should Contain  ${Files}    getenforce
    Should Contain  ${Files}    ldd-version
    Should Contain  ${Files}    route
    Should Contain  ${Files}    ip-route
    Should Contain  ${Files}    dmesg
    Should Contain  ${Files}    env
    Should Contain  ${Files}    ss
    Should Contain  ${Files}    uptime
    Should Contain  ${Files}    mount
    Should Contain  ${Files}    pstree
    Should Contain  ${Files}    lsmod
    Should Contain  ${Files}    lspci
    Should Contain  ${Files}    ListAllFilesInSSPLDir
    Should Contain  ${Files}    DiskSpaceOfSSPL
    Should Contain  ${Files}    ifconfig
    Should Contain  ${Files}    ip-addr
    Should Contain  ${Files}    sysctl
    Should Contain  ${Files}    rpm-pkgs
    Should Contain  ${Files}    dpkg-pkgs
    Should Contain  ${Files}    yum-pkgs
    Should Contain  ${Files}    zypper-pkgs
    Should Contain  ${Files}    apt-pkgs
    Should Contain  ${Files}    ldconfig
    Should Contain  ${Files}    systemctl-status-sophos-spl
    Should Contain  ${Files}    systemctl-status-sophos-spl-update


Check Diagnose Output For System Files
    ${Files} =  List Files In Directory  /tmp/DiagnoseOutput/SystemFiles

    ${ExpectedFilesOnOs} =  get_expected_files_for_platform

    List Should Contain Sub List  ${Files}    ${ExpectedFilesOnOs}

Mimic MDR Files
    [Documentation]  Creates directories and files to simulate logs and interesting files created by MDR Plugin
    [Arguments]     ${installLocation}
    Create Directory  ${installLocation}/plugins/mtr/dbos/data/logs
    Create File  ${installLocation}/plugins/mtr/dbos/data/logs/some.log.file
    Create File  ${installLocation}/plugins/mtr/dbos/data/osquery.flags
    Create File  ${installLocation}/plugins/mtr/VERSION.ini
    Create File  ${installLocation}/plugins/mtr/dbos/data/VERSION.ini