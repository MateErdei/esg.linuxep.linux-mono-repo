*** Settings ***
Test Setup      Setup Thininstaller Test
Test Teardown   Teardown

Suite Setup  sdds3 suite setup with fakewarehouse with real base
Suite Teardown   sdds3 suite fake warehouse Teardown

Library     ${LIBS_DIRECTORY}/UpdateServer.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/FakeSDDS3UpdateCacheUtils.py

Library     Process
Library     OperatingSystem

Resource    ../upgrade_product/UpgradeResources.robot
Resource    ./ThinInstallerResources.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../GeneralTeardownResource.robot

Default Tags  THIN_INSTALLER  UPDATE_CACHE

*** Keywords ***
Setup Update Cache
    Clear Warehouse Config
    Generate Install File In Directory     ./tmp/TestInstallFiles/${BASE_RIGID_NAME}
    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ./tmp/TestInstallFiles/    ./tmp/temp_warehouse/
    Generate Warehouse  ${FALSE}
    Start Update Server    1236    ./tmp/temp_warehouse/warehouse_root/
    Can Curl Url    https://localhost:1236/sophos/customer
    Can Curl Url    https://localhost:1236/sophos/warehouse/catalogue/sdds.${BASE_RIGID_NAME}.xml

Teardown
    General Test Teardown
    Stop Update Server
    Stop Proxy Servers
    Stop Local Cloud Server
    Teardown Reset Original Path
    Run Keyword If Test Failed    Dump Thininstaller Log
    Cleanup Files
    Clear Environment Proxy


Set Bad Environment Proxy
    Set Environment Variable  https_proxy  http://localhost:2/
    Set Environment Variable  http_proxy  http://localhost:2/

Clear Environment Proxy
    Remove Environment Variable  https_proxy
    Remove Environment Variable  http_proxy

*** Test Case ***
Thin Installer can install via Update Cache and Fallback from broken update cache
    Require Uninstalled
    Start Local Cloud Server
    Create Default Credentials File  update_caches=localhost:8080,2,1;localhost:1235,1,1
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller  0  force_certs_dir=${SDDS3_DEVCERTS}
    Check Suldownloader Log Contains In Order
        ...  Trying update via update cache: https://localhost:1235
        ...  Trying update via update cache: https://localhost:8080
        ...  Update success

    check_suldownloader_log_should_not_contain  Connecting to update source directly

Thin Installer can install via Update Cache With Bad Proxy
    Set Bad Environment Proxy
    Require Uninstalled
    Start Local Cloud Server
    Create Default Credentials File  update_caches=localhost:8080,2,1;localhost:1235,1,1
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller  0  force_certs_dir=${SDDS3_DEVCERTS}
    Check Suldownloader Log Contains In Order
        ...  Trying update via update cache: https://localhost:1235
        ...  Trying update via update cache: https://localhost:8080
        ...  Update success

    check_suldownloader_log_should_not_contain  Connecting to update source directly
    check_suldownloader_log_should_not_contain  localhost:2


