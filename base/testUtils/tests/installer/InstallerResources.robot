*** Settings ***
Library    Process
Library    OperatingSystem
Library    ../libs/FullInstallerUtils.py
Library    ../libs/CommsComponentUtils.py
Library     ../libs/UpgradeUtils.py

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
    Verify Sophos Users And Sophos Groups Are Created
    Should Exist   ${SOPHOS_INSTALL}/var/ipc
    Should Exist   ${SOPHOS_INSTALL}/logs/base
    Should Exist   ${SOPHOS_INSTALL}/tmp

Require Installed
    ${result}=  SSPL Is Installed
    Run Keyword If   ${result} != True  Require Fresh Install

Run Specific Installer Directly
    [Arguments]    ${installer_path}
    Run Process  chmod  +x  ${installer_path}
    ${result} =  Run Process  ${installer_path}
    Should Be Equal As Strings  ${result.rc}  0

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
    ${result} =  Run Process   pgrep   -f   ${COMMS_COMPONENT}
    Return from keyword if  ${result.rc} == 1
    ${r} =  Run Process  kill -9 ${result.stdout.replace("\n", " ")}  shell=True
    Should Be Equal As Strings  ${r.rc}  0

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
    ${handle}=  Start Process  find ${SOPHOS_INSTALL}/base -not -type d | grep -v python | grep -v primarywarehouse | grep -v primary | grep -v temp_warehouse | grep -v TestInstallFiles | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  find ${SOPHOS_INSTALL}/logs -not -type d | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  find ${SOPHOS_INSTALL}/var -not -type d | grep -v sophos-spl-comms | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  find ${SOPHOS_INSTALL}/bin -not -type d | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    Log  ${result.stderr}

Display All SSPL Plugins Files Installed
    ${handle}=  Start Process  find ${SOPHOS_INSTALL}/plugins/av -not -type d | grep -v lenses | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${SOPHOS_INSTALL}/plugins/mtr -not -type d | grep -v lenses | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${SOPHOS_INSTALL}/plugins/edr -not -type d | grep -v lenses | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${SOPHOS_INSTALL}/plugins/liveresponse -not -type d | grep -v lenses | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${SOPHOS_INSTALL}/plugins/eventjournaler | xargs ls -l  shell=True
    ${result}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  ${result.stdout}
    ${handle}=  Start Process  find ${SOPHOS_INSTALL}/plugins/runtimedetections | xargs ls -l  shell=True
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
    Should Be Equal As Integers     ${result.rc}    0

Check Management Agent Running
    ${result} =     Run Process     pgrep  -f   sophos_managementagent
    Should Be Equal As Integers     ${result.rc}    0

Check SDU Running
    ${result} =     Run Process     pgrep  -f   sdu
    Should Be Equal As Integers     ${result.rc}    0

Check Update Scheduler Running
    ${result} =     Run Process     pgrep  -f   UpdateScheduler
    Should Be Equal As Integers     ${result.rc}    0

Check Telemetry Scheduler Is Running
    ${result} =     Run Process     pgrep  -f   tscheduler
    Should Be Equal As Integers     ${result.rc}    0

Check Telemetry Scheduler Copy Is Running
    ${result} =     Run Process     pgrep  -f   newTscheduler
    Should Be Equal As Integers     ${result.rc}    0

Check Comms Component Is Running
    ${result_is_running} =     Run Process     pgrep  -f   ${COMMS_COMPONENT}
    Log  ${result_is_running.stdout}
    #stdout will have <pid1>\n <pid2>.
    Should Be Equal As Integers  ${result_is_running.stdout.split().__len__()}  2

Check Watchdog Not Running
    ${result} =    Run Process  pgrep  sophos_watchdog
    Should Not Be Equal As Integers    ${result.rc}    0

Check Management Agent not Running
    ${result} =     Run Process     pgrep  -f   sophos_managementagent
    Should Not Be Equal As Integers     ${result.rc}    0

Check Update Scheduler Not Running
    ${result} =     Run Process     pgrep  -f   UpdateScheduler
    Should Not Be Equal As Integers     ${result.rc}    0

Check Telemetry Scheduler Plugin Not Running
    ${result} =    Run Process  pgrep  tscheduler
    Should Not Be Equal As Integers    ${result.rc}    0

Check Comms Component Not Running
    ${result} =    Run Process  pgrep  -f   ${COMMS_COMPONENT}
    Should Not Be Equal As Integers    ${result.rc}    0

Wait For Base Processes To Be Running
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Expected Base Processes Are Running

Check Expected Base Processes Are Running
    Check Watchdog Running
    Check Management Agent Running
    Check Update Scheduler Running
    Check Telemetry Scheduler Is Running
    Check Comms Component Is Running
    Check SDU Running

Check Expected Base Processes Except SDU Are Running
    Check Watchdog Running
    Check Management Agent Running
    Check Update Scheduler Running
    Check Telemetry Scheduler Is Running
    Check Comms Component Is Running

Check Base Processes Are Not Running
    Check Watchdog Not Running
    Check Management Agent not Running
    Check Update Scheduler Not Running
    Check Telemetry Scheduler Plugin Not Running


Verify Sophos Users And Sophos Groups Are Created
    Verify User Created   sophos-spl-user
    Verify Group Created  sophos-spl-group
    Verify User Created   sophos-spl-network
    Verify User Created   sophos-spl-local
    Verify User Created   sophos-spl-updatescheduler

Combine MTR Develop Component Suite
    ${source} =   Set Variable  /tmp/system-product-test-inputs/sspl-mdr-componentsuite
    ${dest} =     Set Variable  /tmp/system-product-test-inputs/sspl-mdr-componentsuite-sdds
    Combine MTR Component Suite  ${source}  ${dest}

Combine MTR 0-6-0 Component Suite
    ${source} =   Set Variable  /tmp/system-product-test-inputs/sspl-mdr-componentsuite-0-6-0
    ${dest} =     Set Variable  /tmp/system-product-test-inputs/sspl-mdr-componentsuite-0-6-0-sdds
    Combine MTR Component Suite  ${source}  ${dest}

Combine MTR Component Suite
     [Arguments]  ${source}  ${dest}

     copy_files_and_folders_from_within_source_folder  ${source}/SDDS-SSPL-DBOS-COMPONENT   ${dest}  1
     copy_files_and_folders_from_within_source_folder  ${source}/SDDS-SSPL-MDR-COMPONENT  ${dest}
     copy_files_and_folders_from_within_source_folder  ${source}/SDDS-SSPL-MDR-COMPONENT-SUITE  ${dest}



Should Not Have A Given Message In Journalctl Since Certain Time
    [Arguments]  ${message}  ${time}
    ${result} =  Run Process  journalctl -o verbose --since "${time}"  shell=True  timeout=20
    Log  ${result.stdout}
    Should Not Contain    ${result.stdout}    ${message}

Should Have A Given Message In Journalctl Since Certain Time
    [Arguments]  ${message}  ${time}
    ${result} =  Run Process  journalctl -o verbose --since "${time}"  shell=True  timeout=20
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Contain    ${result.stdout}    ${message}

Should Have Set KillMode To Mixed
    ${result}=  Run Process  systemctl show sophos-spl | grep KillMode  shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${result1}=  Run Process  systemctl cat sophos-spl   shell=True
    Log  ${result1.stdout}
    Log  ${result1.stderr}
    Run Keyword if    "KillMode" not in "${result.stdout}"    Should Contain  ${result1.stdout}  KillMode=mixed
    Run Keyword if    "KillMode" in "${result.stdout}"    Should Contain  ${result.stdout}  KillMode=mixed

