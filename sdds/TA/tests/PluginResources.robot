*** Settings ***
Library    Collections
Library    OperatingSystem
Library    Process
Library    String

Library    ${LIB_FILES}/LogUtils.py
Library    ${LIB_FILES}/MCSRouter.py
Library    ${LIB_FILES}/OSUtils.py
Library    ${LIB_FILES}/SafeStoreUtils.py

Resource    GeneralUtilsResources.robot


*** Variables ***
${avBin}                    ${AV_DIR}/sbin/av
${safeStoreBin}             ${AV_DIR}/sbin/safestore
${CLSPath}                  ${AV_DIR}/bin/avscanner

${cleanString}                    I am not a virus
${eicarString}                    X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*
${virusDetectedResult}            ${24}

${rtdBin}     ${RTD_DIR}/bin/runtimedetections


*** Keywords ***
Check AV Plugin Permissions
    ${rc}   ${output} =    Run And Return Rc And Output   find ${AV_DIR} -user sophos-spl-user -print
    Should Be Equal As Integers  ${rc}  0
    Should Be Empty  ${output}

Check AV Plugin Installed
    check_suldownloader_log_should_not_contain    Failed to install as setcap is not installed
    File Should Exist   ${CLSPath}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check AV Plugin Running
    Check AV Plugin Permissions
    Check SafeStore Installed Correctly

Check AV Plugin Running
    ${result} =    Run Process    pgrep    -f    ${avBin}
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    0

Enable On Access Via Policy
    ${mark} =  get_on_access_log_mark
    send_policy_file  core  ${SUPPORT_FILES}/CentralXml/CORE-36_oa_enabled.xml
    wait_for_on_access_log_contains_after_mark   On-access scanning enabled  mark=${mark}  timeout=${15}

Check AV Plugin Can Scan Files
    [Arguments]  ${dirtyFile}=/tmp/dirty_excluded_file
    Create File     /tmp/clean_file    ${cleanString}
    Create File     ${dirty_file}    ${eicarString}

    ${rc}   ${output} =    Run And Return Rc And Output    ${CLSPath} /tmp/clean_file ${dirtyFile}
    Should Be Equal As Integers  ${rc}  ${virusDetectedResult}
    Remove File    /tmp/clean_file
    Remove File    ${dirtyFile}

Check On Access Detects Threats
    ${threatPath} =  Set Variable  /tmp/eicar.com
    ${mark} =  get_on_access_log_mark
    Create File     ${threatPath}    ${eicarString}

    wait for on access log contains after mark  detected "${threatPath}" is infected with EICAR-AV-Test  mark=${mark}
    Remove File  ${threatPath}

Wait Until Threat Detector Running
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  2 secs
    ...  Threat Detector Log Contains    Starting USR1 monitor

Check SafeStore Running
    ${result} =    Run Process    pgrep    safestore
    Log  ${result.stderr}
    Log  ${result.stdout}
    Should Be Equal As Integers    ${result.rc}    ${0}

Get SafeStore PID
    ${pid} =     Run Process    pgrep    safestore
    [Return]    ${pid.stdout}

Check SafeStore Database Exists
    Directory Should Exist    ${SAFESTORE_DB_DIR}
    File Exists With Permissions    ${SAFESTORE_DB_PATH}    root    root    -rw-------
    File Exists With Permissions    ${SAFESTORE_DB_PASSWORD_PATH}    root    root    -rw-------

Check SafeStore Database Has Not Changed
    [Arguments]    ${oldDatabaseDirectory}    ${oldDatabaseContent}    ${oldPassword}
    ${currentDatabaseDirectory} =    List Files In Directory    ${SAFESTORE_DB_DIR}
    ${currentPassword} =    Get File    ${SAFESTORE_DB_PASSWORD_PATH}

    # Removing tmp file: https://www.sqlite.org/tempfiles.html
    Remove Values From List    ${oldDatabaseDirectory}    safestore.db-journal
    Should Be Equal    ${oldDatabaseDirectory}    ${currentDatabaseDirectory}

    ${currentDatabaseContent} =    get_contents_of_safestore_database

    Should Be Equal As Strings    ${oldDatabaseContent}    ${currentDatabaseContent}
    Should Be Equal As Strings    ${oldPassword}    ${currentPassword}

Check SafeStore Permissions And Owner
    ${safeStorePid} =    Get SafeStore PID
    ${watchdogPid} =    Run Process    pgrep    sophos_watchdog

    ${user} =    Run Process    ps    -o    user    -p    ${safeStorePid}
    ${group} =    Run Process    ps    -o    group    -p    ${safeStorePid}
    ${watchdogChildPids} =    Run Process    pgrep    -P    ${watchdogPid.stdout}

    Should Contain    ${user.stdout}    root
    Should Contain    ${group.stdout}    root
    Should Contain    ${watchdogChildPids.stdout}    ${safeStorePid}

Check SafeStore Installed Correctly
    File Should Exist    ${safeStoreBin}
    Wait Until Keyword Succeeds
    ...    15 secs
    ...    1 secs
    ...    Check SafeStore Running
    Wait Until Keyword Succeeds
    ...    30 secs
    ...    2 secs
    ...    check_safestore_log_contains    Quarantine Manager initialised OK
    Wait Until Keyword Succeeds
    ...    30 secs
    ...    2 secs
    ...    check_safestore_log_contains    SafeStore started
    Check SafeStore Database Exists
    Check SafeStore Permissions And Owner

Threat Detector Log Contains
    [Arguments]    ${input}
    ${fileContent}=  Get File  ${AV_DIR}/chroot/log/sophos_threat_detector.log
    Should Contain  ${fileContent}    ${input}

Check Event Journaler Executable Running
    ${result} =    Run Process  pgrep eventjournaler | wc -w  shell=true
    Should Be Equal As Integers    ${result.stdout}    1       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Runtime Detections Plugin Is Running
    ${result} =    Run Process  pgrep  -f  ${rtdBin}
    Should Be Equal As Integers    ${result.rc}    0    RuntimeDetections Plugin not running

Check RuntimeDetections Installed Correctly
    Wait For RuntimeDetections to be Installed
#    Verify RTD Component Permissions
#    Verify Running RTD Component Permissions

Wait For RuntimeDetections to be Installed
    Wait Until Created    ${rtdBin}    timeout=40s
    Wait Until Keyword Succeeds
    ...   40 secs
    ...   1 secs
    ...   Runtime Detections Plugin Is Running
    Wait Until Created    ${RTD_DIR}/var/run/sensor.sock    timeout=30s
    
Verify RTD Component Permissions
    ${result} =   Run Process    find ${RTD_DIR} | xargs ls -ld    shell=True   stderr=STDOUT
    Log   ${result.stdout}

    Verify Permissions    ${RTD_DIR}  0o750    root   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/bin                      0o710    root   sophos-spl-group
    Verify Permissions    ${rtdBin}                           0o710    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/bin/uninstall.sh         0o710    root   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/etc                             0o750    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/etc/sensor.yaml                 0o640    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/etc/analytics.yaml              0o640    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/etc/content_rules/rules.yaml    0o640    root   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/log                          0o700    sophos-spl-user   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/log/runtimedetections.log    0o600    sophos-spl-user   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/var  0o750    root   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/var/lib                        0o750    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/var/lib/struct_layouts.json    0o640    root   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/var/lib/bpf                0o750    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/var/lib/bpf/dummy.bpf.o    0o640    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/var/lib/bpf/dummy.bpf.c    0o640    root   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/var/run    0o750    sophos-spl-user   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/VERSION.ini    0o640    root   sophos-spl-group

Verify Running RTD Component Permissions
    Verify Permissions    ${RTD_DIR}/var/run/cache_analytics.yaml    0o640    sophos-spl-user   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/var/run/sensor.sock             0o770    sophos-spl-user   sophos-spl-group

    Verify Permissions    /var/run/sophos   0o700   sophos-spl-user   sophos-spl-group
    Verify Permissions    /run/sophos       0o700   sophos-spl-user   sophos-spl-group

    IF   ${{os.path.isdir("/sys/fs/cgroup/")}}
        IF   ${{os.path.isdir("/sys/fs/cgroup/systemd/runtimedetections/")}}
            Verify Permissions   /sys/fs/cgroup/systemd/runtimedetections   0o755    sophos-spl-user   sophos-spl-group
        ELSE IF   ${{os.path.isdir("/sys/fs/cgroup/runtimedetections/")}}
            Verify Permissions   /sys/fs/cgroup/runtimedetections   0o755    sophos-spl-user   sophos-spl-group
        ELSE
            Fail   Cannot find cgroup directory for runtimedetections
        END
    END