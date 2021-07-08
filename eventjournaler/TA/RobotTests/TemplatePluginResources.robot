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
Example Keyword
    Create File  /tmp/examplefile.txt  "Contents"