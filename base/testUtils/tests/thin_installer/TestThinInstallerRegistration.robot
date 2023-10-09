*** Settings ***
Test Setup      Setup Thininstaller Test
Test Teardown   Thininstaller Test Teardown

Suite Setup    Upgrade Resources Suite Setup

Library     ${LIBS_DIRECTORY}/UpdateServer.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py
Library     Process
Library     DateTime
Library     OperatingSystem

Resource  ../installer/InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource  ThinInstallerResources.robot

Default Tags  THIN_INSTALLER

*** Keywords ***

*** Variables ***

*** Test Case ***
Thin Installer Registers Before Installing
    Start Local Cloud Server
    Run Default Thininstaller  18
    Check Thininstaller Log Contains    Successfully registered with Sophos Central
    Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Exits Without Installing If Registration Fails
    Start Local Cloud Server  --force-fail-registration
    Run Default Thininstaller  51
    Check Thininstaller Log Contains    Failed to register with Sophos Central, aborting installation
    Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Exits Without Installing If JWT Acquisition Fails
    Start Local Cloud Server  --force-fail-jwt
    Run Default Thininstaller  52
    Check Thininstaller Log Contains    Successfully verified connection to Sophos Central
    Check Thininstaller Log Contains    Failed to authenticate with Sophos Central, aborting installation
    Should Not Exist   ${SOPHOS_INSTALL}

Thin Installer Registers With Group
    Start Local Cloud Server
    Run Default Thininstaller With Args  18  --group=testgroupname
    Check Thininstaller Log Contains    Successfully registered with Sophos Central
    Should Not Exist   ${SOPHOS_INSTALL}
    Check Cloud Server Log Contains  <deviceGroup>testgroupname</deviceGroup>

Thin Installer send migrate when uninstalling sav
    Start Local Cloud Server
    Run Default Thininstaller With Args  18  --uninstall-sav
    Check Thininstaller Log Contains    Successfully registered with Sophos Central
    Should Not Exist   ${SOPHOS_INSTALL}
    Check Cloud Server Log Contains  <migratedFromSAV>1</migratedFromSAV>



