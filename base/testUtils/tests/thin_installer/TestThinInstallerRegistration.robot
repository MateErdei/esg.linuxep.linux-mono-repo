*** Settings ***
Test Setup      Setup Thininstaller Test
Test Teardown   Thininstaller Test Teardown

Suite Setup    Thininstaller Suite Setup
Suite Teardown   Thininstaller Suite Teardown


Library     DateTime
Library     OperatingSystem
Library     Process
Library     ${COMMON_TEST_LIBS}/UpdateServer.py
Library     ${COMMON_TEST_LIBS}/ThinInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/OSUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/TemporaryDirectoryManager.py
Library     ${COMMON_TEST_LIBS}/MCSRouter.py
Library     ${COMMON_TEST_LIBS}/CentralUtils.py
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/InstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/SchedulerUpdateResources.robot
Resource    ${COMMON_TEST_ROBOT}/ThinInstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Force Tags  THIN_INSTALLER

*** Keywords ***
Thininstaller Suite Setup
    Upgrade Resources Suite Setup
    ${handle} =    Start Local SDDS3 Server
    Set Suite Variable    ${GL_handle}    ${handle}
    Wait Until Keyword Succeeds  10 secs  1 secs  Can Curl Url    https://localhost:8080

Thininstaller Suite Teardown
    SDDS3 Suite Fake Warehouse Teardown


*** Variables ***

*** Test Case ***
Thin Installer Registers Before Installing
    Start Local Cloud Server
    Run Default Thininstaller  18    cleanup=False    temp_dir_to_unpack_to=${CUSTOM_TEMP_UNPACK_DIR}

    ${compatibilityCheckResults} =  Get File    ${CUSTOM_THININSTALLER_REPORT_LOC}
    log  ${compatibilityCheckResults}
    Should Contain    ${compatibilityCheckResults}    registrationWithCentral = true
    Check Thininstaller Log Contains    Successfully registered with Sophos Central
    Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Exits Without Installing If Registration Fails
    Start Local Cloud Server  --force-fail-registration
    Run Default Thininstaller  51    cleanup=False    temp_dir_to_unpack_to=${CUSTOM_TEMP_UNPACK_DIR}
    ${errorMsg} =    Set Variable    Failed to register with Sophos Central, aborting installation
    Check Thininstaller Log Contains    ${errorMsg}
    ${compatibilityCheckResults} =  Get File    ${CUSTOM_THININSTALLER_REPORT_LOC}
    log  ${compatibilityCheckResults}
    Should Contain    ${compatibilityCheckResults}    registrationWithCentral = false
    Should Contain    ${compatibilityCheckResults}    ${errorMsg}

    Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Fails to Register With Previous Existing Installation
    Run Full Installer
    Should Exist   ${SOPHOS_INSTALL}
    Start Local Cloud Server  --force-fail-registration
    Run Default Thininstaller  6    cleanup=False    temp_dir_to_unpack_to=${CUSTOM_TEMP_UNPACK_DIR}
    ${errorMsg} =    Set Variable    ERROR: Failed to register with Sophos Central with previous existing installation
    Check Thininstaller Log Contains    ${errorMsg}
    ${compatibilityCheckResults} =  Get File    ${CUSTOM_THININSTALLER_REPORT_LOC}
    log  ${compatibilityCheckResults}
    Should Contain    ${compatibilityCheckResults}    registrationWithCentral = false
    Should Contain    ${compatibilityCheckResults}    ${errorMsg}

Thin Installer Exits Without Installing If JWT Acquisition Fails
    Start Local Cloud Server  --force-fail-jwt
    Run Default Thininstaller  52    cleanup=False    temp_dir_to_unpack_to=${CUSTOM_TEMP_UNPACK_DIR}
    ${errorMsg} =    Set Variable    Failed to authenticate with Sophos Central, aborting installation
    Check Thininstaller Log Contains    Successfully verified connection to Sophos Central
    Check Thininstaller Log Contains    ${errorMsg}
    ${compatibilityCheckResults} =  Get File    ${CUSTOM_THININSTALLER_REPORT_LOC}
    log  ${compatibilityCheckResults}
    Should Contain    ${compatibilityCheckResults}    registrationWithCentral = false
    Should Contain    ${compatibilityCheckResults}    ${errorMsg}

    Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Registers With Group
    Start Local Cloud Server
    Run Default Thininstaller With Args  18  --group=testgroupname    cleanup=False    temp_dir_to_unpack_to=${CUSTOM_TEMP_UNPACK_DIR}
    Check Thininstaller Log Contains    Successfully registered with Sophos Central
    Should Not Exist   ${SOPHOS_INSTALL}
    Check Cloud Server Log Contains  <deviceGroup>testgroupname</deviceGroup>
    ${compatibilityCheckResults} =  Get File    ${CUSTOM_THININSTALLER_REPORT_LOC}
    log  ${compatibilityCheckResults}
    Should Contain    ${compatibilityCheckResults}    registrationWithCentral = true

Thin Installer send migrate when uninstalling sav
    Start Local Cloud Server
    Run Default Thininstaller With Args  18  --uninstall-sav    cleanup=False    temp_dir_to_unpack_to=${CUSTOM_TEMP_UNPACK_DIR}
    Check Thininstaller Log Contains    Successfully registered with Sophos Central
    Should Not Exist   ${SOPHOS_INSTALL}
    Check Cloud Server Log Contains  <migratedFromSAV>1</migratedFromSAV>
    ${compatibilityCheckResults} =  Get File    ${CUSTOM_THININSTALLER_REPORT_LOC}
    log  ${compatibilityCheckResults}
    Should Contain    ${compatibilityCheckResults}    registrationWithCentral = true



