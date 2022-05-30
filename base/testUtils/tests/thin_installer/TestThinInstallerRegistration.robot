*** Settings ***
Test Setup      Setup Thininstaller Test
Test Teardown   Teardown

Suite Setup    Suite Setup Without Ostia

Library     ${LIBS_DIRECTORY}/WarehouseGenerator.py
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
    Run Default Thininstaller


