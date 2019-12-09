*** Settings ***
Library         Process
Library         OperatingSystem
#Library         FakeManagement.py


Test Teardown   Remove All
*** Variables ***
${TEST_INPUT_PATH}  /opt/test/inputs/edr
${BASE_SDDS}    ${TEST_INPUT_PATH}/base-sdds
${EDR_SDDS}     ${TEST_INPUT_PATH}/SDDS-COMPONENT
${SOPHOS_INSTALL}   /opt/sophos-spl
${EDR_PLUGIN_PATH}  ${SOPHOS_INSTALL}/plugins/edr
${EDR_LOG_PATH}    ${EDR_PLUGIN_PATH}/log/edr.log

${EDR_IPC_FILE}       ${SOPHOS_INSTALL}/var/ipc/plugins/edr.ipc

*** Test Cases ***
EDR Can Be Installed and Executed By Watchdog
    Mock Base For Component Installed
    Copy EDR Components
    #Extract SystemProductTestOutputTar File


*** Keywords ***
Mock Base For Component Installed
    Create Directory   ${SOPHOS_INSTALL}
    Create Directory   ${SOPHOS_INSTALL}/var/ipc/
    Create Directory   ${SOPHOS_INSTALL}/var/ipc/plugins/
    Create File        ${SOPHOS_INSTALL}/base/etc/logger.conf   "VERBOSITY=DEBUG"


Copy EDR Components
    Copy Directory   ${EDR_SDDS}/files/plugins   ${SOPHOS_INSTALL}
    Copy Directory   ${EDR_SDDS}/files/base/pluginRegistry   ${SOPHOS_INSTALL}
    Run Process   ldconfig   -lN   *.so.*   cwd=${EDR_PLUGIN_PATH}/lib64/   shell=True

Extract SystemProductTestOutputTar File
    Remove Directory    ${TEST_INPUT_PATH}/SystemProductTestOutput   True
    ${result} =   Run Process    tar  xvf   ${TEST_INPUT_PATH}/SystemProductTestOutput.tar.gz   -C  ${TEST_INPUT_PATH}/SystemProductTestOutput
    Log   ${result.stdout}
    Log   ${result.stderr}
    File Should Exist   ${TEST_INPUT_PATH}/SystemProductTestOutput

Remove All
    Run Process   rm   -rf   ${SOPHOS_INSTALL}