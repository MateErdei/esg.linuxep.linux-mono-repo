*** Settings ***
Library    Process
Library    ${LIBS_DIRECTORY}/UpdateServer.py
Library    ${LIBS_DIRECTORY}/WarehouseGenerator.py
Resource    ../upgrade_product/UpgradeResources.robot
Library    ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library    ${LIBS_DIRECTORY}/OSUtils.py
Library    ${LIBS_DIRECTORY}/LogUtils.py
Resource   ./ThinInstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../mcs_router/McsRouterResources.robot

Test Teardown    Thin installer test teardown
Force Tags  LOAD7

*** Keywords ***


Thin installer test teardown
    General Test Teardown
    Stop Update Server
    Stop Proxy Servers
    Run Keyword If Test Failed    Dump Thininstaller Log
    Run Keyword If Test Failed    Dump Warehouse Log

