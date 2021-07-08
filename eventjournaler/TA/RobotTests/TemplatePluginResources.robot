*** Settings ***
Library         Process
Library         OperatingSystem
Library         String

Resource  GeneralResources.robot
*** Variables ***
${BASE_SDDS}                    ${TEST_INPUT_PATH}/base_sdds/
${TEMPLATE_PLUGIN_SDDS}         ${COMPONENT_SDDS}
${SOPHOS_INSTALL}               /opt/sophos-spl/

*** Keywords ***
Run Shell Process
    [Arguments]  ${Command}   ${OnError}   ${timeout}=20s
    ${result} =   Run Process  ${Command}   shell=True   timeout=${timeout}
    Should Be Equal As Integers  ${result.rc}  0   "${OnError}.\nstdout: \n${result.stdout} \n. stderr: \n${result.stderr}"