*** Settings ***
Library         Process
Library         OperatingSystem
Library         String

Library         ../Libs/AVScanner.py
Library         ../Libs/CoreDumps.py
Library         ../Libs/FakeWatchdog.py
Library         ../Libs/FileUtils.py
Library         ../Libs/LockFile.py
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
    Mark On Access Log
    Start On Access without Log check
    Wait Until On Access running with offset

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

    ${policyContent}=    Get File   ${RESOURCES_PATH}/flags_policy/flags_enabled.json
    Send Plugin Policy  av  FLAGS  ${policyContent}

    Wait Until On Access Log Contains With Offset  "oa_enabled":true
    Wait Until On Access Log Contains With Offset  Starting eventReader


On-access Scan Eicar Close
    #${pid} =  Get Robot Pid
    ${filepath} =  Set Variable  /tmp_test/eicar.com

    Mark On Access Log
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    Wait Until On Access Log Contains With Offset  On-close event for ${filepath} from \   timeout=${timeout}
    Wait Until On Access Log Contains With Offset  "${filepath}" is infected with EICAR-AV-Test \   timeout=${timeout}


On-access Scan Eicar Open
    Mark On Access Log

    #${pid} =  Get Robot Pid
    ${filepath} =  Set Variable  /tmp_test/eicar.com
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    Get File   ${filepath}

    Wait Until On Access Log Contains With Offset  On-open event for ${filepath} from    timeout=${timeout}
    Wait Until On Access Log Contains With Offset  "${filepath}" is infected with    timeout=${timeout}


On-access No Eicar Scan
    #${pid} =  Get Robot Pid
    ${filepath} =  Set Variable  /tmp_test/uncaught_eicar.com

    Mark On Access Log
    Create File  ${filepath}  ${EICAR_STRING}
    Register Cleanup  Remove File  ${filepath}

    On Access Log Does Not Contain With Offset  On-close event for ${filepath} from

On-access Scan Peend
    ${threat_file} =   Set Variable   ${RESOURCES_PATH}/file_samples/peend.exe
    ${threat_name} =   Set Variable   PE/ENDTEST
    ${peend} =   Get Binary File  ${threat_file}

    Mark On Access Log
    ${filepath} =  Set Variable  /tmp_test/peend.exe
    Create Binary File   ${filepath}   ${peend}
    Register Cleanup  Remove File  ${filepath}

    Wait Until On Access Log Contains With Offset  On-close event for ${filepath} from \   timeout=${timeout}
    Wait Until On Access Log Contains With Offset  "${filepath}" is infected with ${threat_name} \   timeout=${timeout}

On-access Scan Peend no detect
    ${threat_file} =   Set Variable   ${RESOURCES_PATH}/file_samples/peend.exe
    ${threat_name} =   Set Variable   PE/ENDTEST
    ${peend} =   Get Binary File  ${threat_file}

    Mark On Access Log
    ${filepath} =  Set Variable  /tmp_test/peend.exe
    Create Binary File   ${filepath}   ${peend}
    Register Cleanup  Remove File  ${filepath}

    On Access Log Does Not Contain With Offset  "${filepath}" is infected with ${threat_name} \

