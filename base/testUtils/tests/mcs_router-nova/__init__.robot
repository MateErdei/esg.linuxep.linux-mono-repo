*** Settings ***
Resource  ../mcs_router/McsRouterResources.robot
Resource  McsRouterNovaResources.robot
Library    ${LIBS_DIRECTORY}/MCSRouter.py

Library   ${LIBS_DIRECTORY}/CentralUtils.py
Library   ${LIBS_DIRECTORY}/ProxyUtils.py

Suite Setup      Setup MCS Tests Nova
Suite Teardown   Nova Suite Teardown

Test Teardown   Nova Test Teardown


