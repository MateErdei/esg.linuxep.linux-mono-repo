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
    Run Keyword If Test Failed   Log File   ${COMPONENT_ROOT_PATH}/log/${COMPONENT_NAME}.log
    Run Keyword If Test Failed   Log File   ${FAKEMANAGEMENT_AGENT_LOG_PATH}

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
    Copy Directory   ${COMPONENT_SDDS}/files/plugins   ${SOPHOS_INSTALL}
    ## Change permissions for all executables
    Run Process   chmod -R +x ${COMPONENT_ROOT_PATH}/sbin ${COMPONENT_ROOT_PATH}/bin  shell=True
    Create Directory  ${COMPONENT_ROOT_PATH}/chroot
    Create Directory  ${COMPONENT_ROOT_PATH}/var
    Run Process   ldconfig   -lN   *.so.*   cwd=${COMPONENT_LIB64_DIR}   shell=True