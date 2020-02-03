*** Settings ***
Test Teardown    Teardown
Suite Teardown   Nova Suite Teardown

Library     ${libs_directory}/FullInstallerUtils.py
Library     ${libs_directory}/LogUtils.py
Library     ${libs_directory}/ThinInstallerUtils.py

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
    [Tags]  MANUAL   THININSTALLER
    Require Uninstalled
    Get Thininstaller
    Build Thininstaller From Sections  ${SUPPORT_FILES}/ThinInstallerMiddles/dogfood.txt
    Run Thininstaller With No Env Changes  0

    # Ensure we used the UCs
    Check Thininstaller Log Does Not Contain  http://dci.sophosupd.com/update

    # Even if the install worked (returned a 0) make sure it has not seg faulted
    Check Thininstaller Log Does Not Contain  Segmentation fault

Thin Installer Can install Via Real Update Cache
    Require Uninstalled
    Get Thininstaller
    Build Thininstaller From Sections  ${SUPPORT_FILES}/ThinInstallerMiddles/real_ucmr.txt

    Run Real Thininstaller

    # Ensure we used the UCs
    Check Thininstaller Log Does Not Contain  http://dci.sophosupd.com/update
    Check Thininstaller Log Contains  Successfully download installer from update cache address [sspluc1:8191]
    Check Thininstaller Log Contains  Installation complete, performing post install steps

Thin Installer Can install by Falling Back from Broken Update Cache to Real Update Cache
    Require Uninstalled
    Get Thininstaller
    Build Thininstaller From Sections  ${SUPPORT_FILES}/ThinInstallerMiddles/real_ucmr_first_uc_broken.txt

    Run Real Thininstaller

    # Ensure we used the UCs
    Check Thininstaller Log Contains  Failed to connect to warehouse at https://uncontactableuc:8191/sophos/customer (SUL error is [4-Out of sources])
    Check Thininstaller Log Contains  Successfully download installer from update cache address [sspluc1:8191]
    Check Thininstaller Log Contains  Installation complete, performing post install steps