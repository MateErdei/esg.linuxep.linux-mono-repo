*** Settings ***
Library         Process
Library         OperatingSystem
Library         String
Library         ../Libs/BaseUtils.py
Library         ../Libs/FakeManagement.py
Library         ../Libs/LogUtils.py
Library         ../Libs/OnFail.py

# Uses functions from AVResources, but can't be included because AVResources need variables only defined
# once GlobalSetup.Global Setup Tasks has completed
# Resource    AVResources.robot

Resource    GlobalSetup.robot

*** Keywords ***

Component Test Setup
    Run Keyword And Ignore Error   Empty Directory   ${SOPHOS_INSTALL}/tmp
    Start Fake Management If Required


Component Test TearDown
    # Stop Fake Management
    Terminate All Processes  kill=True
    Wait until AV Plugin not running
    Wait until Threat Detector not running

    Run Keyword If Test Failed  Dump Log   ${COMPONENT_ROOT_PATH}/log/${COMPONENT_NAME}.log
    Run Keyword If Test Failed  Dump Log   ${FAKEMANAGEMENT_AGENT_LOG_PATH}
    Run Keyword If Test Failed  Dump Log   ${THREAT_DETECTOR_LOG_PATH}


Setup Base And Component
    Mock Base Installation
    Setup Component For Testing


Mock Base Installation
    Run Keyword And Ignore Error  BaseUtils.uninstall_sspl_if_installed
    Create Directory   ${SOPHOS_INSTALL}
    Create Directory   ${SOPHOS_INSTALL}/base/lib64
    Create Directory   ${SOPHOS_INSTALL}/base/mcs/action
    Create Directory   ${SOPHOS_INSTALL}/base/mcs/policy
    Create Directory   ${SOPHOS_INSTALL}/base/telemetry/cache
    Copy File  ${BASE_SDDS}/files/base/lib64/libcrypto.so.1.1  ${SOPHOS_INSTALL}/base/lib64/
    Create Directory   ${SOPHOS_INSTALL}/tmp
    Create Directory   ${SOPHOS_INSTALL}/var/ipc
    Create Directory   ${SOPHOS_INSTALL}/var/ipc/plugins
    Set Base Log Level  DEBUG
    Run Process  groupadd  sophos-spl-group
    Run Process  groupadd  sophos-spl-ipc
    Run Process  useradd  sophos-spl-av               --no-create-home  --no-user-group  --gid  sophos-spl-group  --groups  sophos-spl-ipc  --system
    Run Process  useradd  sophos-spl-threat-detector  --no-create-home  --no-user-group  --gid  sophos-spl-group  --system
    Run Process  useradd  sophos-spl-user             --no-create-home  --no-user-group  --gid  sophos-spl-group  --system

Set Base Log Level
    [Arguments]  ${logLevel}
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  [global]\nVERBOSITY=${logLevel}

Set Log Level
    [Arguments]  ${logLevel}
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf.local  [global]\nVERBOSITY=${logLevel}

Bootstrap SUSI From Update Source
    Create File  ${SOPHOS_INSTALL}/base/etc/machine_id.txt  ab7b6758a3ab11ba8a51d25aa06d1cf4
    ${result} =  Run Process   bash  ${TEST_SCRIPTS_PATH}/bin/bootstrap_susi.sh  stderr=STDOUT
    Log  "Bootstrap output: " ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  ${0}


Setup Component For Testing
    Run  pgrep -f sophos-spl | xargs kill -9
    Copy Directory   ${COMPONENT_SDDS}/files/plugins   ${SOPHOS_INSTALL}
    ## Change permissions for all executables
    Run Process  chmod  -R  +x  ${COMPONENT_ROOT_PATH}/sbin  ${COMPONENT_ROOT_PATH}/bin  ${BASH_SCRIPTS_PATH}  shell=True
    Run Process  chmod  +x  ${COMPONENT_ROOT_PATH}/sophos_certs/InstallCertificateToSystem.sh
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/opt/sophos-spl/base/etc
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/opt/sophos-spl/base/update/var
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/etc
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/log
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/tmp
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/var
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/log
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/var
    Create Directory  ${COMPONENT_ROOT_PATH}/var
    Create Directory  ${COMPONENT_ROOT_PATH}/log
    Run Process   chmod  a+rwx
    ...  ${COMPONENT_ROOT_PATH}/chroot/etc
    ...  ${COMPONENT_ROOT_PATH}/log  ${COMPONENT_ROOT_PATH}/chroot/log
    Run Process   ln  -s  /  ${COMPONENT_ROOT_PATH}/chroot${COMPONENT_ROOT_PATH}/chroot
    Run Process   ln  -snf  ${COMPONENT_ROOT_PATH}/chroot/log  ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector
    ${RELATIVE_PATH} =   Replace String Using Regexp   ${COMPONENT_ROOT_PATH}/log   [^/]+   ..
    ${RELATIVE_PATH} =   Strip String   ${RELATIVE_PATH}   characters=/
    Run Process   ln  -snf  ${RELATIVE_PATH}/log  ${COMPONENT_ROOT_PATH}/chroot/${COMPONENT_ROOT_PATH}/log/sophos_threat_detector
    Run Process   ldconfig   -lN   *.so.*   cwd=${COMPONENT_LIB64_DIR}   shell=True
    Run Process   setcap  cap_sys_chroot\=eip  ${COMPONENT_ROOT_PATH}/sbin/sophos_threat_detector_launcher
    Run Process   setcap  cap_dac_read_search\=eip  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
    Bootstrap SUSI From Update Source

Use Fake AVScanner
    Set Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}  true
    Move File  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher_bkp
    Copy File  ${RESOURCES_PATH}/copyScanConfigFilesToTmp.sh   ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
    Run  chmod 755 ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
    register cleanup  Undo Use Fake AVScanner

Undo Use Fake AVScanner
    ${usingFakeAVScanner} =  Get Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}
    Return From Keyword If  '${usingFakeAVScanner}'!='true'

    Set Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}  false
    Remove File  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
    Move File  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher_bkp  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
    Remove Directory  /tmp/config-files-test/  recursive=true

Delete Eicars From Tmp
    Remove File  /tmp_test/*eicar*
