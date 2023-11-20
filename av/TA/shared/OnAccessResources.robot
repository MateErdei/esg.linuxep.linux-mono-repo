*** Settings ***
Library         Process
Library         OperatingSystem
Library         String

Resource       AVResources.robot

Library         ../Libs/AVScanner.py
Library         ../Libs/CoreDumps.py
Library         ../Libs/FakeWatchdog.py
Library         ../Libs/FileSampleObfuscator.py
Library         ../Libs/FileUtils.py
Library         ../Libs/LockFile.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnAccessUtils.py
Library         ../Libs/OnFail.py
Library         ../Libs/OSUtils.py
Library         ../Libs/LogUtils.py
Library         ../Libs/ThreatReportUtils.py


*** Variables ***
${ONACCESS_FLAG_CONFIG}  ${AV_PLUGIN_PATH}/var/oa_flag.json
${ONACCESS_STATUS_FILE}  ${AV_PLUGIN_PATH}/var/on_access_status


*** Keywords ***
Clear On Access Log When Nearly Full
    ${OA_LOG_SIZE}=  Get File Size in MB   ${ON_ACCESS_LOG_PATH}
    IF    ${OA_LOG_SIZE} > ${0.5}
          Dump Log  ${ON_ACCESS_LOG_PATH}
          Remove File    ${ON_ACCESS_LOG_PATH}
          Run Keyword If   ${ON_ACCESS_PLUGIN_HANDLE}   Restart On Access
    END


Dump and Reset Logs
    Register Cleanup   Remove File      ${AV_PLUGIN_PATH}/log/av.log*
    Register Cleanup   Remove File      ${AV_PLUGIN_PATH}/log/soapd.log*
    Register Cleanup   Dump log         ${AV_PLUGIN_PATH}/log/soapd.log


Start On Access And AV With Running Threat Detector
    Start AV and On Access
    Start Fake Management If Required
    FakeWatchdog.Start Sophos Threat Detector Under Fake Watchdog
    Force SUSI to be initialized

Start On Access without Pid check
    [Documentation]  Sometimes we need to start soapd and expect it to immediately exit e.g. for fault injection tests
    Remove Files   /tmp/soapd.stdout  /tmp/soapd.stderr
    ${handle} =  Start Process  ${ON_ACCESS_BIN}   stdout=/tmp/soapd.stdout  stderr=/tmp/soapd.stderr
    Set Suite Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${handle}

Start On Access without Log check
    Remove Files   /tmp/soapd.stdout  /tmp/soapd.stderr
    ${handle} =  Start Process  ${ON_ACCESS_BIN}   stdout=/tmp/soapd.stdout  stderr=/tmp/soapd.stderr
    ProcessUtils.wait_for_pid  ${ON_ACCESS_BIN}  ${30}
    Set Suite Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${handle}

Start On Access
    ${mark} =  get_on_access_log_mark
    Start On Access without Log check
    Wait Until On Access Running After Mark  ${mark}

Start AV
    Remove Files   /tmp/av.stdout  /tmp/av.stderr
    ${handle} =  Start Process  ${AV_PLUGIN_BIN}   stdout=/tmp/av.stdout  stderr=/tmp/av.stderr
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${handle}

Restart On Access
    Terminate On Access
    Start On Access

Restart AV
    Terminate AV
    Start AV

Restart On Access And AV
    Terminate On Access And AV
    Start AV and On Access

Start AV and On Access
    OnAccessResources.Start AV
    Start On Access

Terminate AV
    Run Keyword If  ${AV_PLUGIN_HANDLE}  Terminate Process  ${AV_PLUGIN_HANDLE}
    Wait Until Keyword Succeeds
        ...  30 secs
        ...  2 secs
        ...  Check AV Plugin Not Running
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${None}

Terminate On Access
    Run Keyword If  ${ON_ACCESS_PLUGIN_HANDLE}  Terminate Process  ${ON_ACCESS_PLUGIN_HANDLE}
    Wait Until Keyword Succeeds
        ...  30 secs
        ...  2 secs
        ...  Check OnAccess Not Running
    Set Suite Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${None}

Terminate On Access And AV
    Terminate On Access
    Terminate AV

Configure On-access Off With Core Policy With Fake Management
    send av policy from file  CORE   ${RESOURCES_PATH}/core_policy/CORE-36_oa_disabled.xml

Check OA disabled in status file
    wait_for_file_to_contain  ${ONACCESS_STATUS_FILE}  disabled  timeout=${1}

Disable OA Scanning
    [Arguments]  ${mark}=${None}
    IF   $mark is None
            ${mark} =  get_on_access_log_mark
    END
    send av policy from file  ${SAV_APPID}    ${RESOURCES_PATH}/SAV-2_policy_OA_disabled.xml
    Configure On-access Off With Core Policy With Fake Management
    send av policy from file  FLAGS  ${RESOURCES_PATH}/flags_policy/flags.json

    wait for on access log contains after mark  On-access enabled: false   mark=${mark}
    wait for on access log contains after mark  Joining eventReader   mark=${mark}
    wait for on access log contains after mark  Stopping the reading of Fanotify events   mark=${mark}
    wait for on access log contains after mark  On-access scanning disabled   mark=${mark}
    wait_for_file_to_contain  ${ONACCESS_STATUS_FILE}  disabled

Wait for OA Scanning enabled in status file
    wait_for_file_to_contain  ${ONACCESS_STATUS_FILE}  enabled

Enable OA Scanning
    [Arguments]  ${mark}=${None}
    IF   $mark is None
            ${mark} =  get_on_access_log_mark
    END
    send av policy from file  ${SAV_APPID}   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    send av policy from file  CORE           ${RESOURCES_PATH}/core_policy/CORE-36_oa_enabled.xml
    send av policy from file  FLAGS          ${RESOURCES_PATH}/flags_policy/flags.json

    wait for on access log contains after mark  Starting eventReader   mark=${mark}    timeout=30
    wait for on access log contains after mark  On-access scanning enabled  mark=${mark}    timeout=30

    # Ensure we have finished processing the policy that has on-close enabled before we proceed
    ${mark2} =  wait for on access log contains after mark  Scanning on-close enabled   mark=${mark}
    wait for on access log contains after mark  Finished ProcessPolicy   mark=${mark2}
    wait for on access log contains after mark   mount points in on-access scanning   mark=${mark2}

    #Ensure that all threads start in order
    ${list}=  create list  Fanotify successfully initialised  Starting eventReader  Starting mountMonitor  mount points in on-access scanning  Starting scanHandler 0  On-access scanning enabled
    check_on_access_log_contains_in_order  ${list}  mark=${mark}
    Wait for OA Scanning enabled in status file


On-access Scan Clean File
    ${cleanfile} =  Set Variable  /tmp_test/cleanfile.txt

    ${oamark} =  get_on_access_log_mark
    Create File   ${cleanfile}   ${CLEAN_STRING}
    Register Cleanup   Remove File   ${cleanfile}
    wait for on access log contains after mark  On-close event for ${cleanfile}  mark=${oamark}

On-access Scan Eicar Close
    #${pid} =  Get Robot Pid
    [Arguments]  ${filepath}=/tmp_test/eicar.com

    ${oamark} =  get_on_access_log_mark
    ${avmark} =  get_av_log_mark
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    wait_for_on_access_log_contains_after_mark  On-close event for ${filepath} from \  mark=${oamark}
    # With DeDup LINUXDAR-5901, eicar can be detected on either the open or the close, so we can't check
    wait_for_on_access_log_contains_after_mark  "${filepath}" is infected with EICAR-AV-Test   mark=${oamark}
    Wait For AV Log Contains After Mark  Found 'EICAR-AV-Test' in '${filepath}'   mark=${avmark}    timeout=30

On-access Scan Eicar Open
    ${cleanfile} =  Set Variable  /tmp_test/cleanfile.txt
    ${dirtyfile} =  Set Variable  /tmp_test/dirtyfile.txt

    # create a file without generating fanotify events
    ${oamark} =  get_on_access_log_mark
    Create File  ${dirtyfile}-excluded  ${EICAR_STRING}
    Register Cleanup   Remove File  ${dirtyfile}-excluded

    #Generate another event we can expect in logs
    Create File  ${cleanfile}  ${CLEAN_STRING}
    Register Cleanup   Remove File   ${cleanfile}
    wait for on access log contains after mark  On-close event for ${cleanfile}  mark=${oamark}

    #Move the first file which doesnt generate a event
    Move File   ${dirtyfile}-excluded   ${dirtyfile}
    Register Cleanup   Remove File   ${dirtyfile}

    ${mark} =  get_on_access_log_mark

    Get File   ${dirtyfile}

    wait_for_on_access_log_contains_after_mark  On-open event for ${dirtyfile} from  mark=${mark}
    wait_for_on_access_log_contains_after_mark  "${dirtyfile}" is infected with      mark=${mark}

On-access Scan On Execute
    [Arguments]  ${check_safestore}=${FALSE}    ${ss_mark}=
    ${mark} =  get_on_access_log_mark

    DeObfuscate File  ${RESOURCES_PATH}/file_samples_obfuscated/linux_elf_threat_obf  ${NORMAL_DIRECTORY}/linux_elf_threat-excluded.exe
    Run Process  chmod  770  ${NORMAL_DIRECTORY}/linux_elf_threat-excluded.exe
    #Ensure the fanotify events for the excluded path have been processed before moving the file.
    On-access Scan Clean File
    Move File   ${NORMAL_DIRECTORY}/linux_elf_threat-excluded.exe   ${NORMAL_DIRECTORY}/linux_elf_threat.exe
    register cleanup    Remove File     ${NORMAL_DIRECTORY}/linux_elf_threat.exe

    ${handle} =     Start Process   ${NORMAL_DIRECTORY}/linux_elf_threat.exe    10

    wait_for_on_access_log_contains_after_mark  linux_elf_threat.exe" is infected with Linux/Test-D (Open)  mark=${mark}
    run keyword if  ${check_safestore}   wait_for_safestore_log_contains_after_mark     Quarantined ${NORMAL_DIRECTORY}/linux_elf_threat.exe successfully   ${ss_mark}

    Process should Be Running   handle=${handle}
    Terminate Process   ${handle}

Create Large Eicar
    ${dirtyfile} =  Set Variable  /tmp_test/dirtyfile_large.txt
    # create a file without generating fanotify events
    ${oamark} =  get_on_access_log_mark
    Create File  ${dirtyfile}-excluded  ${EICAR_STRING}
    Register Cleanup   Remove File  ${dirtyfile}-excluded

    ${result} =  Run Process     truncate   -s      3G     ${dirtyfile}-excluded   shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}

    ${result} =  Run Process     cp  ${dirtyfile}-excluded   ${dirtyfile}   shell=True
    Log  ${result.stdout}
    Log  ${result.stderr}

    Register Cleanup   Remove File   ${dirtyfile}

    wait_for_on_access_log_contains_after_mark  "${dirtyfile}" is infected with EICAR-AV-Test  mark=${oamark}  timeout=180

    #Restart on-access to clear the scanHandler in case the close-write event is still being scanned
    Restart On Access

On-access No Eicar Scan
    ${filepath} =  Set Variable  /tmp_test/uncaught_eicar.com

    ${mark} =  get_on_access_log_mark
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    # Should not be logged:
    check_on_access_log_does_not_contain_before_timeout  On-close event for ${filepath} from  mark=${mark}

On-access Scan Peend
    ${threat_file} =   Set Variable   ${RESOURCES_PATH}/file_samples/peend.exe
    ${threat_name} =   Set Variable   PE/ENDTEST
    ${peend} =   Get Binary File  ${threat_file}

    ${mark} =  get_on_access_log_mark
    ${filepath} =  Set Variable  /tmp_test/peend.exe
    Create Binary File   ${filepath}   ${peend}
    Register Cleanup  Remove File  ${filepath}

    wait_for_on_access_log_contains_after_mark  On-close event for ${filepath} from \  mark=${mark}
    wait_for_on_access_log_contains_after_mark  "${filepath}" is infected with ${threat_name} \   mark=${mark}

On-access Scan Peend no detect
    ${threat_file} =   Set Variable   ${RESOURCES_PATH}/file_samples/peend.exe
    ${threat_name} =   Set Variable   PE/ENDTEST
    ${peend} =   Get Binary File  ${threat_file}

    ${mark} =  get_on_access_log_mark
    ${filepath} =  Set Variable  /tmp_test/peend.exe
    Create Binary File   ${filepath}   ${peend}
    Register Cleanup  Remove File  ${filepath}

    LogUtils.Check On Access Log Does Not Contain Before Timeout  "${filepath}" is infected with ${threat_name} \   mark=${mark}  timeout=5


On-access Scan Multiple Peend
    ${threat_file} =   Set Variable   ${RESOURCES_PATH}/file_samples/peend.exe
    ${threat_name} =   Set Variable   PE/ENDTEST
    ${peend} =   Get Binary File  ${threat_file}

    ${mark} =  get_on_access_log_mark
    Register Cleanup  Empty Directory  /tmp_test/
    FOR  ${item}  IN RANGE  20
        ${filepath} =  Set Variable  /tmp_test/peend-${item}.exe
        Create Binary File   ${filepath}   ${peend}
    END

    FOR  ${item}  IN RANGE  20
        ${filepath} =  Set Variable  /tmp_test/peend-${item}.exe
        wait_for_on_access_log_contains_after_mark  On-close event for ${filepath} from \  mark=${mark}
        wait_for_on_access_log_contains_after_mark  "${filepath}" is infected with ${threat_name} \   mark=${mark}
    END

On-access Scan Multiple Peend no detect
    ${threat_file} =   Set Variable   ${RESOURCES_PATH}/file_samples/peend.exe
    ${threat_name} =   Set Variable   PE/ENDTEST
    ${peend} =   Get Binary File  ${threat_file}

    ${mark} =  get_on_access_log_mark
    Register Cleanup  Empty Directory  /tmp_test/
    FOR  ${item}  IN RANGE  20
        ${filepath} =  Set Variable  /tmp_test/peend-${item}.exe
        Create Binary File   ${filepath}   ${peend}
    END

    LogUtils.Check On Access Log Does Not Contain Before Timeout  \ is infected with ${threat_name} \   mark=${mark}  timeout=5

Remove OA local settings and terminate soapd in product test
    Remove File   ${OA_LOCAL_SETTINGS}
    ${mark} =  get_on_access_log_mark
    Terminate On Access

Set number of scanning threads in product test
    [Arguments]  ${count}
    Set OA local settings   { "numThreads" : ${count} }

Set OA local settings
    [Arguments]  ${contents}
    Create File   ${OA_LOCAL_SETTINGS}   ${contents}
    Register Cleanup If Unique  Remove OA local settings and terminate soapd in product test
    ${mark} =  get_on_access_log_mark
    Restart On Access
    wait for on access log contains after mark   mount points in on-access scanning  mark=${mark}

Generate Clean OnAccess Event
    [Arguments]  ${path}=/tmp_test/cleanfile.txt   ${mark}=${None}

    IF   $mark is None
        ${mark} =  get_on_access_log_mark
    END

    Create File  ${path}  ${CLEAN_STRING}
    Register Cleanup   Remove File   ${path}
    wait for on access log contains after mark  On-close event for ${path}  mark=${mark}