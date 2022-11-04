*** Settings ***
Library         Process
Library         OperatingSystem
Library         String

Resource       AVResources.robot

Library         ../Libs/AVScanner.py
Library         ../Libs/CoreDumps.py
Library         ../Libs/FakeWatchdog.py
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
${timeout}  ${20}


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

Start On Access without Log check
    Remove Files   /tmp/soapd.stdout  /tmp/soapd.stderr
    ${handle} =  Start Process  ${ON_ACCESS_BIN}   stdout=/tmp/soapd.stdout  stderr=/tmp/soapd.stderr
    ProcessUtils.wait_for_pid  ${ON_ACCESS_BIN}  ${30}
    Set Suite Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${handle}

Start On Access
    ${mark} =  get_on_access_log_mark
    Start On Access without Log check
    Wait Until On Access running with offset  ${mark}

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
    Set Suite Variable  ${AV_PLUGIN_HANDLE}  ${None}

Terminate On Access
    Run Keyword If  ${ON_ACCESS_PLUGIN_HANDLE}  Terminate Process  ${ON_ACCESS_PLUGIN_HANDLE}
    Set Suite Variable  ${ON_ACCESS_PLUGIN_HANDLE}  ${None}

Terminate On Access And AV
    Terminate On Access
    Terminate AV

Disable OA Scanning
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_disabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset  "oa_enabled":false
    Wait Until On Access Log Contains With Offset  Joining eventReader


Enable OA Scanning
    ${policyContent}=    Get File   ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml
    Send Plugin Policy  av  sav  ${policyContent}

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_onaccess_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset  "oa_enabled":true
    Wait Until On Access Log Contains With Offset  Starting eventReader
    Wait Until On Access Log Contains With Offset   mount points in on-access scanning


On-access Scan Eicar Close
    #${pid} =  Get Robot Pid
    ${filepath} =  Set Variable  /tmp_test/eicar.com

    ${mark} =  get_on_access_log_mark
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    wait_for_on_access_log_contains_after_mark  On-close event for ${filepath} from \  mark=${mark}  timeout=${timeout}
    wait_for_on_access_log_contains_after_mark  "${filepath}" is infected with EICAR-AV-Test \   mark=${mark}  timeout=${timeout}


On-access Scan Eicar Open
    [Arguments]     ${create-filepath}=/tmp_test/excluded-eicar.com

    Create File  ${create-filepath}  ${EICAR_STRING}

    ${filepath} =  Set Variable  /tmp_test/eicar.com
    Register Cleanup  Remove File  ${filepath}

    Move File   ${create-filepath}  ${filepath}

    ${mark} =  get_on_access_log_mark

    Get File   ${filepath}

    wait_for_on_access_log_contains_after_mark  On-open event for ${filepath} from  mark=${mark}  timeout=${timeout}
    wait_for_on_access_log_contains_after_mark  "${filepath}" is infected with      mark=${mark}  timeout=${timeout}

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

    wait_for_on_access_log_contains_after_mark  On-close event for ${filepath} from \  mark=${mark}  timeout=${timeout}
    wait_for_on_access_log_contains_after_mark  "${filepath}" is infected with ${threat_name} \   mark=${mark}  timeout=${timeout}

On-access Scan Peend no detect
    ${threat_file} =   Set Variable   ${RESOURCES_PATH}/file_samples/peend.exe
    ${threat_name} =   Set Variable   PE/ENDTEST
    ${peend} =   Get Binary File  ${threat_file}

    ${mark} =  get_on_access_log_mark
    ${filepath} =  Set Variable  /tmp_test/peend.exe
    Create Binary File   ${filepath}   ${peend}
    Register Cleanup  Remove File  ${filepath}

    LogUtils.Check On Access Log Does Not Contain Before Timeout  "${filepath}" is infected with ${threat_name} \   mark=${mark}  timeout=5

