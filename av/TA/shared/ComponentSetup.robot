*** Settings ***
Library         Process
Library         OperatingSystem

Resource    GlobalSetup.robot

*** Keywords ***

Component Test Setup
    Run Keyword And Ignore Error   Empty Directory   ${COMPONENT_ROOT_PATH}/log
    Run Keyword And Ignore Error   Empty Directory   ${SOPHOS_INSTALL}/tmp
    Start Fake Management


Component Test TearDown
    Stop Fake Management
    Terminate All Processes  kill=True
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${COMPONENT_ROOT_PATH}/log/${COMPONENT_NAME}.log  encoding_errors=replace
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${FAKEMANAGEMENT_AGENT_LOG_PATH}  encoding_errors=replace
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${THREAT_DETECTOR_LOG_PATH}  encoding_errors=replace


Setup Base And Component
    Mock Base Installation
    Setup Component For Testing


Mock Base Installation
    ${result} =  Run Process  bash  -x  ${SOPHOS_INSTALL}/bin/uninstall.sh  --force
    Log Many  stdout: ${result.stdout}	stderr: ${result.stderr}  rc: ${result.rc}
    Remove Directory   ${SOPHOS_INSTALL}   recursive=True
    Create Directory   ${SOPHOS_INSTALL}
    Create Directory   ${SOPHOS_INSTALL}/base/lib64
    Create Directory   ${SOPHOS_INSTALL}/base/mcs/action
    Create Directory   ${SOPHOS_INSTALL}/base/mcs/policy
    Copy File  ${BASE_SDDS}/files/base/lib64/libcrypto.so.1.1  ${SOPHOS_INSTALL}/base/lib64/
    Create Directory   ${SOPHOS_INSTALL}/tmp
    Create Directory   ${SOPHOS_INSTALL}/var/ipc
    Create Directory   ${SOPHOS_INSTALL}/var/ipc/plugins
    Set Log Level  DEBUG
    Run Process   groupadd  sophos-spl-group

Set Log Level
    [Arguments]  ${logLevel}
    Create File  ${SOPHOS_INSTALL}/base/etc/logger.conf  VERBOSITY=${logLevel}

Setup Component For Testing
    Run  pgrep -f sophos-spl | xargs kill -9
    Copy Directory   ${COMPONENT_SDDS}/files/plugins   ${SOPHOS_INSTALL}
    ## Change permissions for all executables
    Run Process  chmod  -R  +x  ${COMPONENT_ROOT_PATH}/sbin  ${COMPONENT_ROOT_PATH}/bin  ${BASH_SCRIPTS_PATH}  shell=True
    Run Process  chmod  +x  ${COMPONENT_ROOT_PATH}/sophos_certs/InstallCertificateToSystem.sh
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/opt/sophos-spl/base/etc
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/tmp
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot/log
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot${COMPONENT_ROOT_PATH}
    Create Directory  ${COMPONENT_ROOT_PATH}/var
    Create Directory  ${COMPONENT_ROOT_PATH}/log
    Run Process   ln  -s  /  ${COMPONENT_ROOT_PATH}/chroot${COMPONENT_ROOT_PATH}/chroot
    Run Process   ln  -snf  ${COMPONENT_ROOT_PATH}/chroot/log  ${COMPONENT_ROOT_PATH}/log/sophos_threat_detector
    Run Process   ln  -snf  /log  ${COMPONENT_ROOT_PATH}/chroot${COMPONENT_ROOT_PATH}/log/sophos_threat_detector
    Run Process   ldconfig   -lN   *.so.*   cwd=${COMPONENT_LIB64_DIR}   shell=True
    Run Process   ldconfig   -lN   *.so.*   cwd=${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/   shell=True
    Run Process   ldconfig   -lN   *.so.*   cwd=${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/version1   shell=True
    Copy File  ${BASE_SDDS}/files/base/lib64/libz.so.1.*  ${COMPONENT_ROOT_PATH}/chroot/susi/distribution_version/version1/libz.so.1

Use Fake AVScanner
    Set Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}  true
    Move File  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher_bkp
    Copy File  ${RESOURCES_PATH}/copyScanConfigFilesToTmp.sh   ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
    Run  chmod +x ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher

Undo Use Fake AVScanner
    Set Environment Variable  ${USING_FAKE_AV_SCANNER_FLAG}  false
    Remove File  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
    Move File  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher_bkp  ${COMPONENT_ROOT_PATH}/sbin/scheduled_file_walker_launcher
    Remove Directory  /tmp/config-files-test/  recursive=true

Delete Eicars From Tmp
    Remove File  /tmp/*eicar*
