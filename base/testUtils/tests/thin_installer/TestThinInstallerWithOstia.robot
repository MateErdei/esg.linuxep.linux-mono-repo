*** Settings ***
Suite Setup      Ostia Suite Setup
Suite Teardown   Ostia Suite Teardown

Test Setup       Ostia Test Setup
Test Teardown    Ostia Test Teardown

Default Tags  THIN_INSTALLER  OSTIA

Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/WarehouseUtils.py

Resource    ThinInstallerResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    ../upgrade_product/UpgradeResources.robot

*** Variables ***
${BaseOnlyVUTPolicy}                        ${GeneratedWarehousePolicies}/base_only_VUT.xml
${BaseOnlyGAPolicy}                        ${GeneratedWarehousePolicies}/base_only_GA.xml
${BetaOnlyPolicy}                           ${GeneratedWarehousePolicies}/base_beta_only.xml


*** Test Cases ***
Thin Installer Installs Recommended Base When Only a Recommended Version is Available
    [Tags]  THIN_INSTALLER   OSTIA  EXCLUDE_UBUNTU20
    Start Local Cloud Server  --initial-alc-policy  ${BaseOnlyGAPolicy}

    Should Not Exist    ${SOPHOS_INSTALL}

    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseOnlyGAPolicy}  real=True

Thin Installer Installs Recommended Base When the Component Also Has the Beta Tag
    Start Local Cloud Server  --initial-alc-policy  ${BaseOnlyVUTPolicy}

    Should Not Exist    ${SOPHOS_INSTALL}

    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseOnlyVUTPolicy}

Thin Installer Fails to Install Base When Only a Beta Version is Available
    Start Local Cloud Server  --initial-alc-policy  ${BetaOnlyPolicy}

    Should Not Exist    ${SOPHOS_INSTALL}

    Log File  /etc/hosts
    Configure And Run Thininstaller Using Real Warehouse Policy  10  ${BetaOnlyPolicy}