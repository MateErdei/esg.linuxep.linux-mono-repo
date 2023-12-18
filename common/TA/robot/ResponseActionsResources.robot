*** Settings ***
Library     Process
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/OSUtils.py

Resource    GeneralTeardownResource.robot
Resource    GeneralUtilsResources.robot
Resource    McsRouterResources.robot

*** Variables ***
${RESPONSE_ACTIONS_LOG_PATH}   ${SOPHOS_INSTALL}/plugins/responseactions/log/responseactions.log
${ACTIONS_RUNNER_LOG_PATH}   ${SOPHOS_INSTALL}/plugins/responseactions/log/actionrunner.log
${TESTDIR}     /home/vagrant/testdir

${RESPONSE_JSON_PATH}        ${MCS_DIR}/response/
${RESPONSE_JSON}        ${RESPONSE_JSON_PATH}CORE_id1_response.json

*** Keywords ***
Install Response Actions Directly
    ${mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    ${RESPONSE_ACTIONS_SDDS_DIR} =  Get SSPL Response Actions Plugin SDDS
    ${result} =    Run Process  bash -x ${RESPONSE_ACTIONS_SDDS_DIR}/install.sh 2> /tmp/install.log   shell=True
    ${error} =  Get File  /tmp/install.log
    Should Be Equal As Integers    ${result.rc}    0   "Installer failed: Reason ${result.stderr}, ${error}"
    Log  ${error}
    Log  ${result.stderr}
    Log  ${result.stdout}
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Response Actions Executable Running
    wait_for_log_contains_from_mark  ${mark}  Completed initialization of Response Actions

Uninstall Response Actions
    ${result} =  Run Process     ${RESPONSE_ACTIONS_DIR}/bin/uninstall.sh
    Should Be Equal As Strings   ${result.rc}  0
    [Return]  ${result}

Downgrade Response Actions
    ${result} =  Run Process  bash ${RESPONSE_ACTIONS_DIR}/bin/uninstall.sh --downgrade   shell=True   timeout=20s
    Should Be Equal As Integers  ${result.rc}  0   "Failed to downgrade Response Actions.\nstdout: \n${result.stdout}\n. stderr: \n${result.stderr}"

Check Response Actions Executable Running
    ${result} =    Run Process  pgrep responseactions | wc -w  shell=true
    Should Be Equal As Integers    ${result.stdout}    1       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check Response Actions Executable Not Running
    ${result} =    Run Process  pgrep  -a  responseactions
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Stop Response Actions
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  stop  responseactions

Start Response Actions
    Run Process  ${SOPHOS_INSTALL}/bin/wdctl  start  responseactions

Restart Response Actions
    ${mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    Stop Response Actions
    wait_for_log_contains_from_mark  ${mark}  responseactions <> Plugin Finished   30
    ${mark} =  mark_log_size  ${RESPONSE_ACTIONS_LOG_PATH}
    Start Response Actions
    wait_for_log_contains_from_mark  ${mark}  Completed initialization of Response Actions  30

Check Response Actions Installed
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check Log Contains  Entering the main loop  ${RESPONSE_ACTIONS_LOG_PATH}  Response Actions log
    Check Response Actions Executable Running

RA Suite Teardown
    Stop Local Cloud Server
    Require Uninstalled

RA Suite Setup
    Start Local Cloud Server
    Setup_MCS_Cert_Override
    Run Full Installer
    Override LogConf File as Global Level  DEBUG
    Create File  ${MCS_DIR}/certs/ca_env_override_flag

RA Action With Register Suite Setup
    RA Suite Setup
    Register With Local Cloud Server

RA Test Setup
    Require Installed
    HttpsServer.Start Https Server  ${COMMON_TEST_UTILS}/server_certs/server.crt  443  tlsv1_2
    Install Response Actions Directly
    Create File    ${MCS_DIR}/certs/ca_env_override_flag

RA Test Teardown
    General Test Teardown
    Run Cleanup Functions
    Uninstall Response Actions
    Remove file  ${TELEMETRY_OUTPUT_JSON}
    Run Keyword If Test Failed  LogUtils.Dump Log  ${HTTPS_LOG_FILE_PATH}
    Cleanup Telemetry Server
    Remove File  ${EXE_CONFIG_FILE}
    Run Keyword If Test Failed    Dump Cloud Server Log
    Remove File   /tmp/upload.zip

# Run Command tests keywords
RA Run Command Suite Setup
    Start Local Cloud Server
    Setup_MCS_Cert_Override
    ${ma_mark} =  mark_log_size    ${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log
    Run Full Installer
    Install Response Actions Directly
    Create File  ${MCS_DIR}/certs/ca_env_override_flag
    Register With Local Cloud Server
    wait_for_log_contains_from_mark   ${ma_mark}  Policy ALC-1_policy.xml applied to

Simulate Response Action
    [Arguments]    ${action_json_file}    ${id_suffix}=id1    ${sophosInstall}=${SOPHOS_INSTALL}
    ${tmp_action_file} =   Set Variable  ${sophosInstall}/tmp/action.json

    Copy File   ${action_json_file}  ${tmp_action_file}
    ${result} =  Run Process  chown sophos-spl-user:sophos-spl-group ${tmp_action_file}   shell=True
    Should Be Equal As Integers    ${result.rc}    0  Failed to replace permission to file. Reason: ${result.stderr}
    Move File   ${tmp_action_file}  ${sophosInstall}/base/mcs/action/CORE_${id_suffix}_request_2030-02-27T13:45:35.699544Z_144444000000004.json


Require Filesystem
    [Arguments]   ${fs_type}
    ${file_does_not_exist} =  Does File Not Exist  /proc/filesystems
    Pass Execution If    ${file_does_not_exist}  /proc/filesystems does not exist - cannot determine supported filesystems
    ${file_does_not_contain} =  Does File Not Contain  /proc/filesystems  ${fs_type}
    Pass Execution If    ${file_does_not_contain}  ${fs_type} is not a supported filesystem with this kernel - skipping test

Copy And Extract Image
    [Arguments]  ${imagename}
    ${imagetarfile} =  Set Variable  ${SUPPORT_FILES}/FileSystemTypeImages/${imagename}.img.tgz

    ${UNPACK_DIRECTORY} =  Set Variable  ${TESTDIR}/${imagename}.IMG
    Create Directory  ${UNPACK_DIRECTORY}
    ${result} =  Run Process  tar  xvzf  ${imagetarfile}  -C  ${UNPACK_DIRECTORY}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  ${0}
    [Return]  ${UNPACK_DIRECTORY}/${imagename}.img

Mount Image Read Only
    [Arguments]  ${where}  ${image}  ${type}  ${opts}=loop
    Create Directory  ${where}
    ${result} =  Run Process  mount  -t  ${type}  -ro  ${opts}  ${image}  ${where}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  ${0}

Mount Image
    [Arguments]  ${where}  ${image}  ${type}  ${opts}=loop
    Create Directory  ${where}
    ${result} =  Run Process  mount  -t  ${type}  -o  ${opts}  ${image}  ${where}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers  ${result.rc}  ${0}

Unmount Image Internal
    [Arguments]  ${where}
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Run Shell Process   umount ${where}   OnError=Failed to unmount local NFS share

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  2 secs
    ...  Remove Directory    ${where}  recursive=True
