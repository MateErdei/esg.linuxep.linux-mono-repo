*** Settings ***
Resource  ../mcs_router/McsRouterResources.robot
Resource  McsRouterNovaResources.robot
Library    ${libs_directory}/MCSRouter.py

Library   ${libs_directory}/CentralUtils.py
Library   ${libs_directory}/ProxyUtils.py

Suite Setup      Setup MCS Tests Nova
Suite Teardown   Nova Suite Teardown

Test Teardown   Nova Test Teardown


