*** Settings ***
Library     OperatingSystem
Library     Process
Library     String

Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/ProcessUtils.py
Library     ${COMMON_TEST_LIBS}/UpgradeUtils.py
Library     ${COMMON_TEST_LIBS}/Watchdog.py

Resource    GeneralTeardownResource.robot
Resource    McsRouterResources.robot

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
    Verify Sophos Users And Sophos Groups Are Created
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
    ${result} =  Run Process   pgrep   sdu
    Run Keyword If  ${result.rc} == 0  Run Process  kill  -9  ${result.stdout}
    ${result} =  Run Process   pgrep   liveresponse
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
    Check Process Running As User And Group  ${pid}  sophos-spl-user

Check Process Running As Sophosspl Local And Group
    [Arguments]    ${pid}
    Check Process Running As User And Group  ${pid}  sophos-spl-local

Check Process Running As User And Group
    [Arguments]    ${pid}  ${User}
    ${runningUid} =  Run Process    ps  -o  uid  -p  ${pid}
    Log  ${runningUid.stdout}
    ${runningUid} =  Get Regexp Matches  ${runningUid.stdout}  [0-9]+
    Log  ${runningUid[0]}
    ${runningGid} =  Run Process    ps  -o  gid  -p  ${pid}
    Log  ${runningGid.stdout}
    ${runningGid} =  Get Regexp Matches  ${runningGid.stdout}  [0-9]+
    Log  ${runningGid[0]}
    ${idOutput} =  Run Process  id  ${User}
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
    [Arguments]    ${installDir}=${SOPHOS_INSTALL}
    ${handle}=  Start Process  find ${installDir}/base -not -type d | grep -v python | grep -v primarywarehouse | grep -v primary | grep -v temp_warehouse | grep -v TestInstallFiles | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  find ${installDir}/logs -not -type d | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  find ${installDir}/bin -not -type d | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  find ${installDir}/var -not -type d | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  find ${installDir}/etc -not -type d | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}

Display All SSPL Plugins Files Installed
    [Arguments]    ${installDir}=${SOPHOS_INSTALL}
    ${handle}=  Start Process  find ${installDir}/plugins/av -not -type d | grep -v lenses | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${installDir}/plugins/deviceisolation | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${installDir}/plugins/edr -not -type d | grep -v lenses | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${installDir}/plugins/liveresponse -not -type d | grep -v lenses | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${installDir}/plugins/eventjournaler | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${installDir}/plugins/runtimedetections | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${installDir}/plugins/heartbeat | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${installDir}/plugins/responseactions | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}

Display List Files Dash L in Directory 
   [Arguments]  ${DirPath}
   ${result}=    Run Process  ls -l ${DirPath}  shell=True
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
    Should Be Equal As Integers     ${result.rc}    ${0}

Check Management Agent Running
    ${result} =     Run Process     pgrep  -f   sophos_managementagent
    Should Be Equal As Integers     ${result.rc}    ${0}

Check SDU Running
    ${result} =     Run Process     pgrep  -f   sdu
    Should Be Equal As Integers     ${result.rc}    ${0}

Check Update Scheduler Running
    ${result} =     Run Process     pgrep  -f   UpdateScheduler
    Should Be Equal As Integers     ${result.rc}    ${0}

Check Telemetry Scheduler Is Running
    ${result} =     Run Process     pgrep  -f   tscheduler
    Should Be Equal As Integers     ${result.rc}    ${0}

Check Telemetry Scheduler Copy Is Running
    ${result} =     Run Process     pgrep  -f   newTscheduler
    Should Be Equal As Integers     ${result.rc}    ${0}

Check Watchdog Not Running
    ${result} =    Run Process  pgrep  sophos_watchdog
    Should Not Be Equal As Integers    ${result.rc}    ${0}

Check Management Agent not Running
    ${result} =     Run Process     pgrep  -f   sophos_managementagent
    Should Not Be Equal As Integers     ${result.rc}    0

Check Update Scheduler Not Running
    ${result} =     Run Process     pgrep  -f   UpdateScheduler
    Should Not Be Equal As Integers     ${result.rc}    0

Check Telemetry Scheduler Plugin Not Running
    ${result} =    Run Process  pgrep  tscheduler
    Should Not Be Equal As Integers    ${result.rc}    0

Wait For Base Processes To Be Running
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Expected Base Processes Are Running

Wait For Base Processes To Be Running Except MCS Router
    [Arguments]    ${secondsToWait}=10
    Wait Until Keyword Succeeds
    ...  ${secondsToWait} secs
    ...  1 secs
    ...  Check Expected Base Processes Are Running Except MCS Router

Check Expected Base Processes Are Running
    Check Watchdog Running
    Check Management Agent Running
    Check Update Scheduler Running
    Check Telemetry Scheduler Is Running
    Check SDU Running
    Verify Watchdog Config
    Check MCS Router Running

# To be used when side loading - Ignores MCS Router not running because an installation that cannot register will
# succeed but MCS Router will keep exiting because it cannot register, so will not be running.
Check Expected Base Processes Are Running Except MCS Router
    Check Watchdog Running
    Check Management Agent Running
    Check Update Scheduler Running
    Check Telemetry Scheduler Is Running
    Check SDU Running
    Verify Watchdog Config

Check Expected Base Processes Except SDU Are Running
    Check Watchdog Running
    Check Management Agent Running
    Check Update Scheduler Running
    Check Telemetry Scheduler Is Running

Check Base Processes Are Not Running
    Check Watchdog Not Running
    Check Management Agent not Running
    Check Update Scheduler Not Running
    Check Telemetry Scheduler Plugin Not Running


Verify Sophos Users And Sophos Groups Are Created
    Verify User Created   sophos-spl-user
    Verify Group Created  sophos-spl-group
    Verify User Created   sophos-spl-local
    Verify User Created   sophos-spl-updatescheduler

Should Not Have A Given Message In Journalctl Since Certain Time
    [Arguments]  ${message}  ${time}
    ${result} =  Run Process  journalctl  -o  verbose  --since  ${time}  timeout=20  env:SYSTEMD_PAGER=cat
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers    ${result.rc}     ${0}
    Should Not Contain    ${result.stdout}    ${message}

Should Have A Given Message In Journalctl Since Certain Time
    [Arguments]  ${message}  ${time}
    ${result} =  Run Process  journalctl  -o  verbose  --since  ${time}  timeout=20  env:SYSTEMD_PAGER=cat
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers    ${result.rc}     ${0}
    Should Contain    ${result.stdout}    ${message}

Should Have A Stopped Sophos Message In Journalctl Since Certain Time
    [Arguments]   ${time}
    ${result} =  Run Process  journalctl  -o  verbose  --since  ${time}  -u  sophos-spl  timeout=20  env:SYSTEMD_PAGER=cat
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers    ${result.rc}     ${0}
    Should Contain Any   ${result.stdout}   Stopped Sophos Linux Protection.    Stopping Sophos Linux Protection...    Stopped sophos-spl.service

Should Have Set KillMode To Mixed
    ${result}=  Run Process  systemctl show sophos-spl | grep KillMode  shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${result1}=  Run Process  systemctl cat sophos-spl   shell=True
    Log  ${result1.stdout}
    Log  ${result1.stderr}
    Run Keyword if    "KillMode" not in "${result.stdout}"    Should Contain  ${result1.stdout}  KillMode=mixed
    Run Keyword if    "KillMode" in "${result.stdout}"    Should Contain  ${result.stdout}  KillMode=mixed

Verify AV Processes Are Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/av/sbin/av
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/av/sbin/safestore
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/av/sbin/soapd
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/av/sbin/sophos_threat_detector

Wait For AV To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For Av Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify AV Processes Are Running    ${install_path}

Verify Base Processes Are Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/base/bin/sophos_management
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/base/bin/python3
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/base/bin/UpdateScheduler
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/base/bin/tscheduler
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/base/bin/sophos_watchdog

Wait For Base To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For Base Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify Base Processes Are Running    ${install_path}

Verify DI Process Is Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    # TODO: once DI is in current shipping, remove extra check
    ${binary_file_exists} =    Check File Exists    ${install_path}/plugins/deviceisolation/bin/deviceisolation
    IF    ${binary_file_exists}
        Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/deviceisolation/bin/deviceisolation
    END

Wait For DI To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For Deviceisolation Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify DI Process Is Running    ${install_path}

Verify EDR Processes Are Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/edr/bin/edr
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/edr/bin/osqueryd

Wait For EDR To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For EDR Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify EDR Processes Are Running    ${install_path}

Verify EJ Process Is Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/eventjournaler/bin/eventjournaler

Wait For EJ To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For EJ Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify EJ Process Is Running    ${install_path}

Verify LR Process Is Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/liveresponse/bin/liveresponse

Wait For LR To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For Liveresponse Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify LR Process Is Running    ${install_path}

Verify RTD Process Is Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/runtimedetections/bin/runtimedetections

Wait For RTD To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For RTD Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify RTD Process Is Running    ${install_path}

Verify RA Process Is Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/responseactions/bin/responseactions

Wait For RA To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For Response Action Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify RA Process Is Running    ${install_path}

Check Plugin Processes Are Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Verify AV Processes Are Running    ${install_path}
    Verify Base Processes Are Running    ${install_path}
    Verify DI Process Is Running    ${install_path}
    Verify EDR Processes Are Running    ${install_path}
    Verify EJ Process Is Running    ${install_path}
    Verify LR Process Is Running    ${install_path}
    Verify RTD Process Is Running    ${install_path}
    Verify RA Process Is Running    ${install_path}

Wait For Plugins To Be Ready
    [Arguments]    ${log_marks}    ${old_version}=${FALSE}     ${install_path}=${SOPHOS_INSTALL}
    Wait For Plugins Logs To Indicate Plugins Are Ready    log_marks=${log_marks}    timeout=${60}    oldcode=${old_version}
    Check Plugin Processes Are Running    ${install_path}