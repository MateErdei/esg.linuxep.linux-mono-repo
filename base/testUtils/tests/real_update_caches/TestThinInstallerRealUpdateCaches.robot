*** Settings ***
Test Teardown    Teardown
Suite Teardown   Nova Suite Teardown

Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py

Resource    ../GeneralTeardownResource.robot
Resource  ../mcs_router-nova/McsRouterNovaResources.robot

Default Tags  CENTRAL  MCS  THIN_INSTALLER  UPDATE_CACHE  EXCLUDE_AWS

*** Keywords ***
Teardown
    General Test Teardown
    Uninstall SAV
    Run Keyword If Test Failed    Dump Thininstaller Log

*** Test Case ***
Thin Installer Can install Via Real Dogfood Update Cache With Middle Which Caused Segfault In Sul
    [Tags]  MANUAL   THIN_INSTALLER
    Require Uninstalled
    Get Thininstaller
    Build Thininstaller From Sections  ${SUPPORT_FILES}/ThinInstallerMiddles/dogfood.txt
    Run Thininstaller With No Env Changes  0

    # Ensure we used the UCs
    Check Thininstaller Log Does Not Contain  http://dci.sophosupd.com/update

    # Even if the install worked (returned a 0) make sure it has not seg faulted
    Check Thininstaller Log Does Not Contain  Segmentation fault

