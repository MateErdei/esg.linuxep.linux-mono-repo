*** Settings ***
Default Tags    MANUAL
Test Timeout    NONE
Resource  SDDS3Resources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource    ../thin_installer/ThinInstallerResources.robot
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py
Test Setup   Generate Fake sdds3 warehouse
Test Teardown  sdds3 Teardown
*** Keywords ***
sdds3 Teardown
    Clean up fake warehouse
    General Test Teardown
    Stop Update Server
    Stop Proxy Servers
    Run Keyword If Test Failed   Dump Cloud Server Log
    Stop Local Cloud Server
    Cleanup Local Cloud Server Logs
    Teardown Reset Original Path
    Run Keyword If Test Failed    Dump Thininstaller Log
    Remove Thininstaller Log
    Stop Local SDDS3 Server
*** Test Cases ***
Run Sdds warehouse
    ${Files} =  List Files In Directory  ${SDDS3_FAKEFLAGS}
    Log  ${Files}
    ${Files} =  List Files In Directory  ${SDDS3_FAKEPACKAGES}
    Log  ${Files}
    ${Files} =  List Files In Directory  ${SDDS3_FAKESUITES}
    Log  ${Files}
    Regenerate Certificates
    Set Local CA Environment Variable
    Generate Local Ssl Certs If They Dont Exist
    Install Local SSL Server Cert To System
    Start Local Cloud Server
    Get Thininstaller
    Create Default Credentials File
    Build Default Creds Thininstaller From Sections
    ${handle}=  Start Local SDDS3 server with fake files
    Set Suite Variable    ${GL_handle}    ${handle}
    Run Default Thininstaller  0  force_certs_dir=${SUPPORT_FILES}/sophos_certs