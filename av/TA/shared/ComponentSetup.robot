*** Settings ***
Library         Process
Library         OperatingSystem
Library         ../Libs/FakeManagement.py


*** Keywords ***

Component Test Setup
    Run Keyword And Ignore Error   Empty Directory   ${COMPONENT_ROOT_PATH}/log
    Run Keyword And Ignore Error   Empty Directory   ${SOPHOS_INSTALL}/tmp
    Start Fake Management


Component Test TearDown
    Stop Fake Management
    Terminate All Processes  kill=True
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${COMPONENT_ROOT_PATH}/log/${COMPONENT_NAME}.log
    Run Keyword If Test Failed  Run Keyword And Ignore Error  Log File   ${FAKEMANAGEMENT_AGENT_LOG_PATH}

Setup Base And Component
    Mock Base Installation
    Setup Component For Testing


Mock Base Installation
    Remove Directory   ${SOPHOS_INSTALL}   recursive=True
    Create Directory   ${SOPHOS_INSTALL}
    Create Directory   ${SOPHOS_INSTALL}/tmp
    Create Directory   ${SOPHOS_INSTALL}/var/ipc
    Create Directory   ${SOPHOS_INSTALL}/var/ipc/plugins
    Create File        ${SOPHOS_INSTALL}/base/etc/logger.conf   VERBOSITY=DEBUG
    Run Process   groupadd  sophos-spl-group


Setup Component For Testing
    Run  pgrep -f sophos-spl | xargs kill -9
    Copy Directory   ${COMPONENT_SDDS}/files/plugins   ${SOPHOS_INSTALL}
    ## Change permissions for all executables
    Run Process   chmod  -R  +x  ${COMPONENT_ROOT_PATH}/sbin  ${COMPONENT_ROOT_PATH}/bin  ${BASH_SCRIPTS_PATH}  shell=True
    Run Process  chmod  +x  ${COMPONENT_ROOT_PATH}/sophos_certs/InstallCertificateToSystem.sh
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot
    Create Directory  ${COMPONENT_ROOT_PATH}/var
    Run Process   ldconfig   -lN   *.so.*   cwd=${COMPONENT_LIB64_DIR}   shell=True

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
