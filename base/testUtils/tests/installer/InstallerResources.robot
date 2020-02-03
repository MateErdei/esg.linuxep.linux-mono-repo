*** Settings ***
Library    Process
Library    OperatingSystem
Library    ${libs_directory}/FullInstallerUtils.py

Resource  ../GeneralTeardownResource.robot

*** Variables ***
${MCSROUTER_PROCESS_NAME}  /opt/sophos-spl/base/bin/python3 -m mcsrouter

*** Keywords ***
Ensure Uninstalled
    Uninstall SSPL

Require Fresh Install
    Ensure Uninstalled
    Should Not Exist   ${SOPHOS_INSTALL}
    Kill Sophos Processes
    Run Full Installer
    Should Exist   ${SOPHOS_INSTALL}
    Verify Group Created
    Verify User Created
    Should Exist   ${SOPHOS_INSTALL}/var/ipc
    Should Exist   ${SOPHOS_INSTALL}/logs/base
    Should Exist   ${SOPHOS_INSTALL}/tmp

Require Installed
    ${result}=  SSPL Is Installed
    Run Keyword If   ${result} != True  Require Fresh Install

Kill Sophos Processes That Arent Watchdog
    ${result} =  Run Process   pgrep   -f   ${MANAGEMENT_AGENT}
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    ${result} =  Run Process   pgrep   mtr
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    ${result} =  Run Process   pgrep   -f   ${MCSROUTER_PROCESS_NAME}
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    ${result} =  Run Process   pgrep   UpdateScheduler
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    ${result} =  Run Process   pgrep   tscheduler
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}

Kill Sophos Processes
    ${result} =  Run Process   pgrep   sophos_watchdog
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    Kill Sophos Processes That Arent Watchdog

Stop Watchdog
    Run Process    systemctl    stop    ${WATCHDOG_SERVICE}

Start Watchdog
    Run Process    systemctl    start    ${WATCHDOG_SERVICE}

Check Process Running As Sophosspl User And Group
    [Arguments]    ${pid}
    ${runningUid} =  Run Process    ps  -o  uid  -p  ${pid}
    Log  ${runningUid.stdout}
    ${runningUid} =  Get Regexp Matches  ${runningUid.stdout}  [0-9]+
    Log  ${runningUid[0]}
    ${runningGid} =  Run Process    ps  -o  gid  -p  ${pid}
    Log  ${runningGid.stdout}
    ${runningGid} =  Get Regexp Matches  ${runningGid.stdout}  [0-9]+
    Log  ${runningGid[0]}
    ${idOutput} =  Run Process  id  sophos-spl-user
    ${expectedUid} =  Get Regexp Matches  ${idOutput.stdout}  uid=([^\(]*)  1
    Log  ${expectedUid[0]}
    ${expectedGid} =  Get Regexp Matches  ${idOutput.stdout}  gid=([^\(]*)  1
    Log  ${expectedGid[0]}
    Should Be Equal As Integers  ${expectedUid[0]}  ${runningUid[0]}  Process not running with correct uid
    Should Be Equal As Integers  ${expectedGid[0]}  ${runningGid[0]}  Process not running with correct gid

Set Sophos Install Environment Variable
    [Arguments]  ${New_SOPHOS_INSTALL}
    ${SOPHOS_INSTALL_ENVIRONMENT} =  Get Environment Variable  SOPHOS_INSTALL  NOT_SET
    Set Suite Variable  ${SOPHOS_INSTALL_ENVIRONMENT_CACHE}  ${SOPHOS_INSTALL_ENVIRONMENT}
    Set Environment Variable  SOPHOS_INSTALL  ${New_SOPHOS_INSTALL}

Reset Sophos Install Environment Variable
    ${VarExist}=  Get Variable Value  ${SOPHOS_INSTALL_ENVIRONMENT_CACHE}  Cache_Not_Defined
    Run Keyword If  "${VarExist}" != "Cache_Not_Defined"  Reset Sophos Install Environment Variable Cache Exists

Reset Sophos Install Environment Variable Cache Exists
    Run Keyword If  "${SOPHOS_INSTALL_ENVIRONMENT_CACHE}" == "NOT_SET"  Remove Environment Variable  SOPHOS_INSTALL
    ...         ELSE  Set Environment Variable  SOPHOS_INSTALL  ${SOPHOS_INSTALL_ENVIRONMENT_CACHE}

Display All SSPL Files Installed
    ${handle}=  Start Process  find ${SOPHOS_INSTALL} | grep -v python | grep -v primarywarehouse | grep -v temp_warehouse | grep -v TestInstallFiles | grep -v lenses   shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}

Display All SSPL Plugins Files Installed
    ${handle}=  Start Process  find   ${SOPHOS_INSTALL}/plugins
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}


Display All tmp Files Present
    ${handle}=  Start Process  find   ./tmp
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}

Check SSPL systemd files are correct
    ## Check systemd files
    ${SystemdPath}  Run Keyword If  os.path.isdir("/lib/systemd/system/")  Set Variable   /lib/systemd/system/
    ...  ELSE  Set Variable   /usr/lib/systemd/system/
    ${SystemdInfo}=  Run Process  find  ${SystemdPath}  -name  sophos-spl*  -type  f  -exec  stat  -c  %a, %G, %U, %n  {}  +
    Should Be Equal As Integers  ${SystemdInfo.rc}  0  Command to find all systemd files and permissions failed
    Create File  ./tmp/NewSystemdInfo  ${SystemdInfo.stdout}
    #Make sure expected and tested results are both sorted on test system as ubuntu minimum gives a different result for a sort
    ${SystemdInfo}=  Run Process  sort  ./tmp/NewSystemdInfo
    ${ExpectedSystemdInfo}=  Run Process  sort  ${ROBOT_TESTS_DIR}/installer/InstallSet/SystemdInfo
    Should Be Equal As Strings  ${ExpectedSystemdInfo.stdout}  ${SystemdInfo.stdout}

Check Watchdog Running
    ${result} =     Run Process     pgrep  -f   sophos_watchdog
    Should Be Equal As Integers     ${result.rc}    0

Check Management Agent Running
    ${result} =     Run Process     pgrep  -f   sophos_managementagent
    Should Be Equal As Integers     ${result.rc}    0

Check Update Scheduler Running
    ${result} =     Run Process     pgrep  -f   UpdateScheduler
    Should Be Equal As Integers     ${result.rc}    0

Check Telemetry Scheduler Is Running
    ${result} =     Run Process     pgrep  -f   tscheduler
    Should Be Equal As Integers     ${result.rc}    0

Check Expected Base Processes Are Running
    Check Watchdog Running
    Check Management Agent Running
    Check Update Scheduler Running
    Check Telemetry Scheduler Is Running