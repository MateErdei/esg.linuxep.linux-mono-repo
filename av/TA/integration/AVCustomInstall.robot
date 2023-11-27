*** Settings ***
Default Tags    INTEGRATION    CUSTOM_INSTALL_PATH  TAP_PARALLEL2

Library    OperatingSystem
Library    Process

Library    ../Libs/BaseInteractionTools/PolicyUtils.py
Library    ../Libs/BaseUtils.py
Library    ../Libs/CoreDumps.py
Library    ../Libs/ExclusionHelper.py
Library    ../Libs/LogUtils.py
Library    ../Libs/OnAccessUtils.py
Library    ../Libs/OnFail.py
Library    ../Libs/ProcessUtils.py
Library    ../Libs/TeardownTools.py

Resource    ../shared/AVResources.robot
Resource    ../shared/ErrorMarkers.robot

Suite Setup    uninstall_sspl_if_installed

Test Setup    Custom Install Test Setup
Test Teardown    Custom Install Test Teardown

*** Variables ***
${CUSTOM_INSTALL_LOCATION}            /etc/sophos-spl
${CUSTOM_AV_PLUGIN_PATH}              ${CUSTOM_INSTALL_LOCATION}/plugins/av
${CUSTOM_AV_BINARY}                   ${CUSTOM_AV_PLUGIN_PATH}/sbin/av
${CUSTOM_THREAT_DETECTOR_BINARY}      ${CUSTOM_AV_PLUGIN_PATH}/sbin/sophos_threat_detector

${CUSTOM_AV_LOG_PATH}                 ${CUSTOM_AV_PLUGIN_PATH}/log/av.log
${CUSTOM_ON_ACCESS_LOG_PATH}          ${CUSTOM_AV_PLUGIN_PATH}/log/soapd.log
${CUSTOM_CLOUDSCAN_LOG_PATH}          ${CUSTOM_AV_PLUGIN_PATH}/log/Sophos Cloud Scheduled Scan.log
${CUSTOM_SAFESTORE_LOG_PATH}          ${CUSTOM_AV_PLUGIN_PATH}/log/safestore.log
${CUSTOM_THREAT_DETECTOR_LOG_PATH}    ${CUSTOM_AV_PLUGIN_PATH}/chroot/log/sophos_threat_detector.log


*** Test Cases ***
CLS Can Detect Threats When SPL Is Installed In Custom Location
    Directory Should Exist    ${CUSTOM_INSTALL_LOCATION}
    Directory Should Not Exist    ${SOPHOS_INSTALL}
    check avscanner can detect eicar

Scheduled Scan Can Run When SPL Is Installed In Custom Location
    Configure Scan Now for Custom Install
    ${file_name} =       Set Variable   TestScanNowCustomInstall
    Create File  /tmp_test/${file_name}  ${EICAR_STRING}
    Register Cleanup    Remove File  /tmp_test/${file_name}

    ${av_mark} =  mark_log_size    ${CUSTOM_AV_LOG_PATH}

    register_cleanup_if_unique    Empty Directory    ${CUSTOM_INSTALL_LOCATION}/base/mcs/action
    ${savActionFilename} =  Generate Random String
    Copy File  ${RESOURCES_PATH}/ScanNow_Action.xml  ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/SAV_action_${savActionFilename}.xml
    Move File  ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/SAV_action_${savActionFilename}.xml  ${CUSTOM_INSTALL_LOCATION}/base/mcs/action/SAV_action_${savActionFilename}.xml

    wait_for_log_contains_after_mark  ${CUSTOM_AV_LOG_PATH}    Starting scan Scan Now    ${av_mark}    ${40}
    wait_for_log_contains_after_mark  ${CUSTOM_AV_LOG_PATH}    Completed scan            ${av_mark}    ${180}
    wait_for_log_contains_after_mark  ${CUSTOM_AV_LOG_PATH}    Sending scan complete     ${av_mark}
    wait_for_log_contains_after_mark  ${CUSTOM_AV_LOG_PATH}    Found 'EICAR-AV-Test' in '/tmp_test/${file_name}'             ${av_mark}

On Access Excludes Binaries Within AV Plugin Directory When Scanning In Custom Location
    Enable OnAccess In Custom Install Location

    # Eicar files created by ../manual/createEicar.sh
    @{tmpEicarFiles} =    Create List
    ...    /tmp/altdir/eicar.com
    ...    /tmp/altdir/subdir/eicar.com
    ...    /tmp/dir/eicar.com
    ...    /tmp/dir/subdir/eicar.com
    ...    /tmp/eicar.com
    ...    /tmp/eicar.com.com
    ...    /tmp/eicar.exe
    ...    /tmp/noteicar.com
    ${result} =    Run Process     which bash    shell=true
    Copy File    ${result.stdout}    ${CUSTOM_AV_PLUGIN_PATH}
    register_cleanup  Remove File  ${CUSTOM_AV_PLUGIN_PATH}/bash

    ${oa_mark1} =    mark_log_size    ${CUSTOM_ON_ACCESS_LOG_PATH}
    ${result} =    Run Process    ${CUSTOM_AV_PLUGIN_PATH}/bash    ${TEST_SCRIPTS_PATH}/manual/slowCreateEicar.sh
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  ${0}

    FOR  ${eicarPath}  IN  @{tmpEicarFiles}
        check_log_does_not_contain_after_mark    ${CUSTOM_ON_ACCESS_LOG_PATH}    On-open event for ${eicarPath} from Process    ${oa_mark1}
        check_log_does_not_contain_after_mark    ${CUSTOM_ON_ACCESS_LOG_PATH}    On-close event for ${eicarPath} from Process   ${oa_mark1}
        check_log_does_not_contain_after_mark    ${CUSTOM_ON_ACCESS_LOG_PATH}    detected "${eicarPath}" is infected with EICAR-AV-Test    ${oa_mark1}
        Remove File    ${eicarPath}
    END

    ${oa_mark2} =    mark_log_size    ${CUSTOM_ON_ACCESS_LOG_PATH}
    ${result} =    Run Process    /bin/bash    ${TEST_SCRIPTS_PATH}/manual/createEicar.sh
    Should Be Equal As Integers  ${result.rc}  ${0}

    FOR  ${eicarPath}  IN  @{tmpEicarFiles}
        Wait For Log Contains From Mark    ${oa_mark2}   On-open event for ${eicarPath} from Process
        Wait For Log Contains From Mark    ${oa_mark2}   On-close event for ${eicarPath} from Process
        Wait For Log Contains From Mark    ${oa_mark2}   detected "${eicarPath}" is infected with EICAR-AV-Test
        # Even though the docs say Remove File should pass if the file does not exist, this is not the case here.
        # Ignore the error, as this file will have been cleaned up by safestore but try and remove it for failure cases.
        register_cleanup    Run Keyword And Ignore Error    Remove File    ${eicarPath}
    END

    # Re-check the excluded items haven't appeared - we have definitely scanned other files since
    FOR  ${eicarPath}  IN  @{tmpEicarFiles}
        check_log_does_not_contain_after_mark  ${CUSTOM_ON_ACCESS_LOG_PATH}  On-open event for ${eicarPath} from Process ${CUSTOM_AV_PLUGIN_PATH}/bash    ${oa_mark1}
        check_log_does_not_contain_after_mark  ${CUSTOM_ON_ACCESS_LOG_PATH}  On-close event for ${eicarPath} from Process ${CUSTOM_AV_PLUGIN_PATH}/bash   ${oa_mark1}
    END


*** Keywords ***
Custom Install Test Setup
    uninstall_sspl_if_installed
    Directory Should Not Exist  ${SOPHOS_INSTALL}
    uninstall_sspl_from_custom_location_if_installed    ${CUSTOM_INSTALL_LOCATION}

    Set Environment Variable    SOPHOS_INSTALL    ${CUSTOM_INSTALL_LOCATION}
    Set Local Variable    ${SOPHOS_INSTALL}    ${CUSTOM_INSTALL_LOCATION}

    Install Base For Component Tests    ${CUSTOM_INSTALL_LOCATION}
    set_spl_log_level_and_restart_watchdog_if_changed    DEBUG

    ${av_mark} =  mark_log_size    ${CUSTOM_AV_LOG_PATH}

    ${install_log} =  Set Variable   ${AV_INSTALL_LOG}
    ${result} =   Run Process   bash  -x  ${AV_SDDS}/install.sh   timeout=60s  stderr=STDOUT   stdout=${install_log}
    ${log_contents} =  Get File  ${install_log}
    File Log Should Not Contain  ${AV_INSTALL_LOG}  chown: cannot access
    Should Be Equal As Integers  ${result.rc}  ${0}   "Failed to install plugin.\noutput: \n${log_contents}"

    Copy File    ${RESOURCES_PATH}/ALC_Policy.xml    ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/ALC-1_policy.xml
    Run Process    chmod    666    ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/ALC-1_policy.xml
    Move File    ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/ALC-1_policy.xml    ${CUSTOM_INSTALL_LOCATION}/base/mcs/policy/ALC-1_policy.xml

    ${tmpSAVPolicyFileName} =  Set Variable    TempSAVpolicy.xml
    create_sav_policy_with_no_scheduled_scan    ${tmpSAVPolicyFileName}
    Copy File    ${RESOURCES_PATH}/${tmpSAVPolicyFileName}  ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/SAV-2_policy.xml
    Run Process    chmod    666    ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/SAV-2_policy.xml
    Move File    ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/SAV-2_policy.xml    ${CUSTOM_INSTALL_LOCATION}/base/mcs/policy/SAV-2_policy.xml

    Copy File  ${RESOURCES_PATH}/flags_policy/flags.json  ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/flags.json
    Run Process  chmod  666  ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/flags.json
    Move File  ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/flags.json  ${CUSTOM_INSTALL_LOCATION}/base/mcs/policy/flags.json

    Remove Directory  /tmp/DiagnoseOutput  true

    ${result} =  Run Process  id  sophos-spl-user  stderr=STDOUT
    Log  "id sophos-spl-user = ${result.stdout}"

    wait_for_pid    ${CUSTOM_AV_BINARY}    ${30}
    wait_for_log_contains_after_mark    ${CUSTOM_AV_LOG_PATH}    Starting Scan Scheduler    ${av_mark}    ${40}
    wait_for_pid    ${CUSTOM_THREAT_DETECTOR_BINARY}    ${30}
    wait_For_Log_contains_after_last_restart    ${CUSTOM_THREAT_DETECTOR_LOG_PATH}    Common <> Starting USR1 monitor    ${60}

    wait_For_Log_contains_after_last_restart    ${CUSTOM_AV_LOG_PATH}    Starting sophos_threat_detector monitor
    wait_For_Log_contains_after_last_restart    ${CUSTOM_THREAT_DETECTOR_LOG_PATH}    ProcessControlServer starting listening on socket: /var/process_control_socket    ${120}

Custom Install Test Teardown
    Run Keyword If Test Failed    Display All SSPL Files Installed    ${CUSTOM_INSTALL_LOCATION}

    Register On Fail  dump log    ${CUSTOM_INSTALL_LOCATION}/logs/base/watchdog.log
    Register On Fail  dump log    ${CUSTOM_INSTALL_LOCATION}/logs/base/sophosspl/sophos_managementagent.log

    Register On Fail  dump log    ${AV_INSTALL_LOG}
    register On Fail  dump log    ${CUSTOM_AV_LOG_PATH}
    Register On Fail  dump log    ${CUSTOM_THREAT_DETECTOR_LOG_PATH}
    Register On Fail  dump log    ${CUSTOM_ON_ACCESS_LOG_PATH}
    Register On Fail  dump log    ${CUSTOM_SAFESTORE_LOG_PATH}

    Register On Fail    dump_dmesg
    Register On Fail    dump_log    /var/log/syslog
    Register On Fail    dump_log    /var/log/messages
    Register On Fail    analyse_Journalctl    print_always=${True}

    run_cleanup_functions
    Run Keyword And Ignore Error    Empty Directory    ${SCAN_DIRECTORY}

    #mark errors related to scheduled scans being forcefully terminated at the end of a test
    Exclude Failed To Scan Multiple Files Cloud                    ${CUSTOM_CLOUDSCAN_LOG_PATH}
    Exclude UnixSocket Interrupted System Call Error Cloud Scan    ${CUSTOM_CLOUDSCAN_LOG_PATH}
    Exclude Watchdog Log Unable To Open File Error                 ${CUSTOM_INSTALL_LOCATION}/logs/base/watchdog.log
    Exclude Expected Sweep Errors                                  ${CUSTOM_AV_PLUGIN_PATH}/chroot/log
    #mark generic errors caused by the lack of a central connection/warehouse/subscription
    Exclude Invalid Settings No Primary Product           ${CUSTOM_INSTALL_LOCATION}/logs/base/suldownloader.log
    Exclude Configuration Data Invalid                    ${CUSTOM_INSTALL_LOCATION}/logs/base/suldownloader.log
    Exclude Failed To connect To Warehouse Error          ${CUSTOM_INSTALL_LOCATION}/logs/base/suldownloader.log
    Exclude CustomerID Failed To Read Error               ${CUSTOM_AV_PLUGIN_PATH}/chroot/log
    Exclude Invalid Day From Policy                       ${CUSTOM_AV_LOG_PATH}
    Exclude Core Not In Policy Features                   ${CUSTOM_INSTALL_LOCATION}/logs/base/sophosspl/updatescheduler.log
    Exclude SPL Base Not In Subscription Of The Policy    ${CUSTOM_INSTALL_LOCATION}/logs/base/sophosspl/updatescheduler.log
    Exclude UpdateScheduler Fails                         ${CUSTOM_INSTALL_LOCATION}/logs/base/sophosspl/updatescheduler.log
    Exclude MCS Router is dead                            ${CUSTOM_INSTALL_LOCATION}/logs/base/watchdog.log

    check_all_product_logs_do_not_contain_error
    run_failure_functions_if_failed

    Set Environment Variable  SOPHOS_INSTALL                /opt/sophos-spl

    uninstall_sspl_from_custom_location_if_installed    ${CUSTOM_INSTALL_LOCATION}

Configure Scan Now for Custom Install
    ${av_mark} =  mark_log_size    ${CUSTOM_AV_LOG_PATH}

    Fill_In_On_Demand_Posix_Exclusions    ${RESOURCES_PATH}/SAV_Policy_Scan_Now.xml  ${RESOURCES_PATH}/FilledIn.xml
    give_policy_unique_revision_id    ${RESOURCES_PATH}/FilledIn.xml    FilledInWithRevId.xml
    Copy File    ${RESOURCES_PATH}/FilledInWithRevId.xml    ${CUSTOM_INSTALL_LOCATION}/base/mcs/policy/SAV-2_policy.xml

    wait_for_log_contains_after_mark    ${CUSTOM_AV_LOG_PATH}    Configured number of Scheduled Scans    ${av_mark}    ${180}

Enable OnAccess In Custom Install Location
    ${oa_mark} =    mark_log_size    ${CUSTOM_ON_ACCESS_LOG_PATH}
    Copy File    ${RESOURCES_PATH}/SAV-2_policy_OA_enabled.xml  ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/SAV-2_policy.xml
    Run Process    chmod    666    ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/SAV-2_policy.xml
    Move File    ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/SAV-2_policy.xml    ${CUSTOM_INSTALL_LOCATION}/base/mcs/policy/SAV-2_policy.xml

    Copy File  ${RESOURCES_PATH}/core_policy/CORE-36_oa_enabled.xml  ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/CORE-36_policy.xml
    Run Process  chmod  666  ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/CORE-36_policy.xml
    Move File  ${CUSTOM_INSTALL_LOCATION}/base/mcs/tmp/CORE-36_policy.xml  ${CUSTOM_INSTALL_LOCATION}/base/mcs/policy/CORE-36_policy.xml

    wait_for_on_access_to_be_enabled    ${oa_mark}    log_path=${CUSTOM_ON_ACCESS_LOG_PATH}
