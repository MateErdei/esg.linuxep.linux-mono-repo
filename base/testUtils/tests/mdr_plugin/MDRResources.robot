*** Settings ***
Library     Process
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  ../GeneralTeardownResource.robot

*** Variables ***
${SophosMDRExe}                     ${SOPHOS_INSTALL}/plugins/mtr/dbos/SophosMTR
${MDRPluginPolicyLocation}          ${SOPHOS_INSTALL}/plugins/mtr/var/policy/mtr.xml
${MDR_STATUS_XML}                   ${SOPHOS_INSTALL}/base/mcs/status/MDR_status.xml
${MDRBasePolicy}                    ${SOPHOS_INSTALL}/base/mcs/policy/MDR_policy.xml
${MDR_PLUGIN_PATH}                  ${SOPHOS_INSTALL}/plugins/mtr

${IPC_FILE} =                       ${SOPHOS_INSTALL}/var/ipc/plugins/mtr.ipc
${CACHED_STATUS_XML} =              ${SOPHOS_INSTALL}/base/mcs/status/cache/MDR.xml
${MDR_LOG_FILE} =                   ${MDR_PLUGIN_PATH}/log/mtr.log
${MTR_MESSAGE_RELAY_CONFIG_FILE}    ${SOPHOS_INSTALL}/var/sophosspl/mcs/current_connection/MessageRelayConfig.xml
${MCS_POLICY_CONFIG}                ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config
${MCS_CONFIG}                       ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
${COMPONENT_TEMP_DIR}  /tmp/mdr_component
${OSqueryPurgeMsg}     Osquery database watcher purge completed

*** Keywords ***

Install MDR Directly
    ${MDR_SDDS_DIR} =  Get SSPL MDR Plugin SDDS
    ${result} =    Run Process  bash -x ${MDR_SDDS_DIR}/install.sh 2> /tmp/install_mdr.log   shell=True
    ${stderr} =  Get File  /tmp/install_mdr.log
    Should Be Equal As Integers    ${result.rc}    20   "Installer failed: Reason ${result.stderr} ${stderr}"
    Log  ${result.stdout}
    Log  ${stderr}
    Log  ${result.stderr}
    Check MDR Plugin Installed

Install MDR Directly with Fake SophosMTR
    ${MDR_SDDS_DIR} =  Get SSPL MDR Plugin SDDS
    ${result} =    Run Process  ${MDR_SDDS_DIR}/install.sh
    # installer will report version copy error since osquery is not present
    Should Be Equal As Integers    ${result.rc}    20
    Log  ${result.stdout}
    Log  ${result.stderr}
    Create Fake MDR Executable
    Check MDR Plugin Installed

Check MDR Plugin Installed
    File Should Exist   ${MDR_PLUGIN_PATH}/bin/mtr
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check MDR Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  MDR Plugin Log Contains  mtr <> Entering the main loop

Check SophosMTR installed
      File Should Exist   ${MDR_PLUGIN_PATH}/dbos/SophosMTR

Check EAP MDR Plugin Installed
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mdr/bin/mdr
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check EAP MDR Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  MDR Plugin Log Contains  mdr <> Entering the main loop

Check MDR Plugin Uninstalled
    Directory Should Not Exist  ${MDR_PLUGIN_PATH}
    File Should Not Exist       ${MTR_MESSAGE_RELAY_CONFIG_FILE}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check MDR Plugin Not Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check SophosMTR Executable Not Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check MTR Osquery Executable Not Running

Check MDR component suite running
    Directory Should Exist  ${MDR_PLUGIN_PATH}
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check MDR Plugin Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check SophosMTR Executable Running
    Wait Until Keyword Succeeds
    ...  15 secs
    ...  1 secs
    ...  Check MTR Osquery Executable Running

Check MDR Plugin Running
    ${result} =    Run Process  pgrep  mtr
    Should Be Equal As Integers    ${result.rc}    0

Check EAP MDR Plugin Running
    ${result} =    Run Process  pgrep  mtr
    Should Be Equal As In   tegers    ${result.rc}    0

Check MDR Plugin Not Running
    ${result} =    Run Process  pgrep  mtr
    Should Not Be Equal As Integers    ${result.rc}    0   msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Wait Until SophosMTR Executable Running
    [Arguments]  ${WaitInSecs}=10
    Wait Until Keyword Succeeds
    ...  ${WaitInSecs} secs
    ...  1 secs
    ...  Check SophosMTR Executable Running

Wait Until EDR and MTR OSQuery Running
    [Arguments]  ${WaitInSecs}=10
    Wait Until Keyword Succeeds
    ...  ${WaitInSecs} secs
    ...  1 secs
    ...  Check EDR And MTR Osquery Executable Running

Wait Until OSQuery Running
    [Arguments]  ${WaitInSecs}=10
    Wait Until Keyword Succeeds
    ...  ${WaitInSecs} secs
    ...  1 secs
    ...  Check Osquery Executable Running

Wait Until EDR Running
    [Arguments]  ${WaitInSecs}=10
    Wait Until Keyword Succeeds
    ...  ${WaitInSecs} secs
    ...  1 secs
    ...  Check EDR Executable Running

Check SophosMTR Executable Running
    ${result} =    Run Process  pgrep  SophosMTR
    Should Be Equal As Integers    ${result.rc}    0       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check SophosMTR Executable Not Running
    ${result} =    Run Process  pgrep  SophosMTR
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0   msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check EDR And MTR Osquery Executable Running
    #Check ALL EDR And MTR osquery instances are running
    ${result} =    Run Process  pgrep osquery | wc -w  shell=true
    Should Be Equal As Integers    ${result.stdout}    4       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check Osquery Executable Running
    #Check both osquery instances are running
    ${result} =    Run Process  pgrep osquery | wc -w  shell=true
    Should Be Equal As Integers    ${result.stdout}    2       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check MTR Osquery Executable Running
    #Check both osquery instances are running
    ${result} =    Run Process  pgrep -a osquery | grep plugins/mtr | wc -l  shell=true
    Should Be Equal As Integers    ${result.stdout}    2       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check MTR Osquery Executable Not Running
    ${result} =    Run Process  pgrep  -a  osquery | grep plugins/mtr
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check Osquery Executable Not Running
    ${result} =    Run Process  pgrep  -a  osquery
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check EDR Executable Running
    #Check both osquery instances are running
    ${result} =    Run Process  pgrep edr | wc -w  shell=true
    Should Be Equal As Integers    ${result.stdout}    1       msg="stdout:${result.stdout}\nerr: ${result.stderr}"

Check EDR Executable Not Running
    ${result} =    Run Process  pgrep  -a  edr
    Run Keyword If  ${result.rc}==0   Report On Process   ${result.stdout}
    Should Not Be Equal As Integers    ${result.rc}    0     msg="stdout:${result.stdout}\nerr: ${result.stderr}"

MDR Plugin Log Contains
    [Arguments]  ${TextToFind}
    File Should Exist  ${MDR_LOG_FILE}
    ${fileContent}=  Get File  ${MDR_LOG_FILE}
    Should Contain  ${fileContent}    ${TextToFind}


Kill MDR Plugin
    ${result} =    Run Process  pgrep  mtr
    Log  ${result.stdout}
    Run Keyword If  ${result.rc} == 0  Run Process  kill   ${result.stdout}


Kill SophosMTR Executable
    ${result} =    Run Process  pgrep  SophosMTR
    Run Process  kill  -9  ${result.stdout}
    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  Check SophosMTR Executable Not Running

Kill OSQuery
    ${result} =  Run Process  pgrep osquery | xargs kill -9  shell=true
    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  Check Osquery Executable Not Running

Create Fake MDR Executable Contents
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...    echo 'MDR Perpetual Executable started'
    ...    while true; do
    ...       sleep 3
    ...    done
    ...    \
    [Return]  ${script}

Create Fake MDR Executable
    ${script} =  Create Fake MDR Executable Contents
    Create Directory   ${MDR_PLUGIN_PATH}/dbos/
    Create File   ${SophosMDRExe}    content=${script}
    Run Process  chmod  u+x  ${SophosMDRExe}

Disable MDR Executable And Restart MDR Plugin
    Wait for MDR Executable To Be Running
    Stop MDR Plugin
    Create File   ${MDR_Disabled_File}    content not used
    Start MDR Plugin
    Sleep  5  #Wait to make sure MDR executable is not started
    MDR Plugin Log Contains    MTR is disabled: SophosMTR will not be started.
    Check SophosMTR Executable Not Running

Wait for MDR Executable To Be Running
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check SophosMTR Executable Running

Stop MDR Plugin
    ${result} =    Run Process   ${SOPHOS_INSTALL}/bin/wdctl  stop  mtr
    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  Check MDR Plugin Not Running
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check SophosMTR Executable Not Running

Start MDR Plugin
    ${result} =    Run Process   ${SOPHOS_INSTALL}/bin/wdctl  start  mtr
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 sec
    ...  Check MDR Plugin Running

Send Default MDR Policy And Check it is Written With Correct Permissions
    ${MDRPolicy} =  Get File  ${SUPPORT_FILES}/CentralXml/MDR_policy.xml
    Create File  ${SOPHOS_INSTALL}/tmp/MDR_policy.xml  ${MDRPolicy}
    Move File  ${SOPHOS_INSTALL}/tmp/MDR_policy.xml  ${SOPHOS_INSTALL}/base/mcs/policy
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  1 secs
    ...  Check MDR Policy Has Been Written And RevID Logged  TESTPOLICY1
    ${MDRComparison} =  Get File  ${MDRPluginPolicyLocation}
    Should Be Equal  ${MDRPolicy}  ${MDRComparison}
    Check File Permissions Match  -rw-------  ${MDR_PLUGIN_PATH}/var/policy/mtr.xml

Restore Connection Between EndPoint and FleetManager
    Run Process   iptables   -D   INPUT   -s   10.10.10.10   -j   DROP

Block Connection Between EndPoint And FleetManager
    #we do not want a connection to dbos backend for testing that dbos executable will start osquery,
    #without a connection to backend the dbos executable will continute running for 10-20 minutes
    Run Process   iptables   -A   INPUT   -s   10.10.10.10   -j   DROP

Check MDR Component Suite Installed Correctly
    Check MDR Plugin Installed
    File Should Exist  ${MDR_PLUGIN_PATH}/dbos/data/certificate.crt
    File Should Exist  ${MDR_PLUGIN_PATH}/dbos/data/certificate.crt.0
    File Should Exist  ${MDR_PLUGIN_PATH}/dbos/SophosMTR
    File Should Exist  ${MDR_PLUGIN_PATH}/dbos/SophosMTR.0
    File Should Exist  ${MDR_PLUGIN_PATH}/osquery/bin/osquery
    File Should Exist  ${MDR_PLUGIN_PATH}/osquery/bin/osquery.0
    Directory Should Exist  ${MDR_PLUGIN_PATH}/var/policy

Check SSPL Installed
    Should Exist    ${SOPHOS_INSTALL}

Wait Until MDR Installed
    Wait Until Keyword Succeeds
    ...  60
    ...  1
    ...  Check MDR Component Suite Installed Correctly

Wait Until MDR Uninstalled
    Wait Until Keyword Succeeds
    ...  60
    ...  1
    ...  Check MDR Plugin Uninstalled

Check MDR Policy Has Been Written And RevID Logged
    [Arguments]  ${RevID}
    File Should Exist   ${MDRPluginPolicyLocation}
    MDR Plugin Log Contains  RevID of the new policy: ${RevID}

Uninstall MDR Plugin And Return Result
    ${result} =  Run Process     ${MDR_PLUGIN_PATH}/bin/uninstall.sh
    Should Be Equal As Strings   ${result.rc}  0
    [Return]  ${result}

Uninstall MDR Plugin
    # A hack to be able to see and report the mtr.log after the plugin uninstallation.
    # tail -f will keep reference to the mtr.log inode. Even after the mtr directory is removed.
    # this is to allow detecting issues with the uninstaller.
    ${handle}=  Start Process  tail -f ${MDR_LOG_FILE}  shell=True
    ${result} =  Run Process     bash -x ${MDR_PLUGIN_PATH}/bin/uninstall.sh 2> /tmp/mdr_uninstall.log   shell=True
    ${stderr} =  Get File   /tmp/mdr_uninstall.log
    Log  ${stderr}
    ${logresult}=  Wait For Process  ${handle}  timeout=30  on_timeout=kill
    Log  "Last mdr logs:\n${logresult.stdout}"
    Should Be Equal As Strings   ${result.rc}  0   msg="output:${result.stdout}\nerror:${result.stderr}."


MDR Test Teardown
    General Test Teardown
    Run Keyword And Ignore Error  Remove File  ${MDRBasePolicy}
    Restore Connection Between EndPoint and FleetManager
    Run Keyword And Ignore Error  Uninstall MDR Plugin
    Check MDR Plugin Uninstalled
    # prevent issues from current run to have knock on effect on others
    Kill MDR Plugin
    # the check mdr plugin uninstalled may failed and logs of base may be necessary.
    Run Keyword If Test Failed    Dump All Logs
    Run Keyword If Test Failed    Require Fresh Install


Report on Pid
    [Arguments]  ${pid}
    Log File  /proc/${pid}/status
    Log File  /proc/${pid}/stat

Install Directly From Component Suite
    ${MDR_COMPONENT_SUITE} =  Get SSPL MDR Component Suite
    ${result} =  Run Process    rsync   -r   -v   ${MDR_COMPONENT_SUITE.mdr_plugin.sdds}/  ${COMPONENT_TEMP_DIR}/
    Should Be Equal As Integers    ${result.rc}    0    Failed to copy ${MDR_COMPONENT_SUITE.mdr_plugin.sdds} to ${COMPONENT_TEMP_DIR}
    ${result} =  Run Process    rsync   -r   -v   ${MDR_COMPONENT_SUITE.dbos.sdds}/  ${COMPONENT_TEMP_DIR}/
    Should Be Equal As Integers    ${result.rc}    0    Failed to copy ${MDR_COMPONENT_SUITE.dbos.sdds} to ${COMPONENT_TEMP_DIR}
    ${result} =  Run Process    rsync   -r   -v   ${MDR_COMPONENT_SUITE.osquery.sdds}/  ${COMPONENT_TEMP_DIR}/
    Should Be Equal As Integers    ${result.rc}    0    Failed to copy ${MDR_COMPONENT_SUITE.osquery.sdds} to ${COMPONENT_TEMP_DIR}
    ${result} =  Run Process    rsync   -r   -v   ${MDR_COMPONENT_SUITE.mdr_suite.sdds}/  ${COMPONENT_TEMP_DIR}/
    Should Be Equal As Integers    ${result.rc}    0    Failed to copy ${MDR_COMPONENT_SUITE.mdr_suite.sdds} to ${COMPONENT_TEMP_DIR}
    LIST FILES IN DIRECTORY  ${COMPONENT_TEMP_DIR}
    ${result} =  Run Process    ${COMPONENT_TEMP_DIR}/install.sh
    Should Be Equal As Integers  ${result.rc}  0    "MDR installer failed: stdout: ${result.stdout} stderr: ${result.stderr}"

Insert MTR Policy
    ${MDRPolicy} =  Get File  ${SUPPORT_FILES}/CentralXml/MDR_policy.xml
    Create File  ${SOPHOS_INSTALL}/tmp/MDR_policy.xml  ${MDRPolicy}
    Move File  ${SOPHOS_INSTALL}/tmp/MDR_policy.xml  ${SOPHOS_INSTALL}/base/mcs/policy

Trigger Osquery Database Purge
    ${result} =  Run Process  dd if\=/dev/urandom bs\=1024 count\=200 | split -a 4 -b 1k - /opt/sophos-spl/plugins/mtr/dbos/data/osquery.db/mtrtelemetrytest_file.  shell=true
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Strings  ${result.rc}  0

Wait For Osquery To Purge Database
    Wait Until Keyword Succeeds
    ...  120s
    ...  10s
    ...  Osquery Watcher Log Should Contain  ${OSqueryPurgeMsg}

Osquery Has Not Purged Database
    Osquery Watcher Log Should Not Contain  ${OSqueryPurgeMsg}

Install MTR From Fake Component Suite

    Block Connection Between EndPoint And FleetManager
    Install Directly From Component Suite

    Check MDR Component Suite Installed Correctly
    Insert MTR Policy

    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/VERSION.ini
    Component Suite Version Ini File Contains Proper Format   ${SOPHOS_INSTALL}/plugins/mtr/dbos/data/VERSION.ini

    Wait Until Keyword Succeeds
    ...  20 secs
    ...  1 secs
    ...  File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/var/policy/mtr.xml
    Wait Until SophosMTR Executable Running  20
    Wait Until Keyword Succeeds
    ...  35 secs
    ...  1 secs
    ...  Check Osquery Executable Running