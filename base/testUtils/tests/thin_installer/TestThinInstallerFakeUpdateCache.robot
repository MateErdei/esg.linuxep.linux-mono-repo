*** Settings ***
Test Teardown   Teardown
Suite Teardown   Refresh HTTPS Certs

Library     ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library     ${LIBS_DIRECTORY}/UpdateServer.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     Process
Library     OperatingSystem
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
    Uninstall SAV
    Run Keyword If Test Failed    Dump Thininstaller Log
    Cleanup Files
    Clear Environment Proxy

Refresh HTTPS Certs
    Cleanup System Ca Certs
    Regenerate HTTPS Certificates
    Copy File   ${SUPPORT_FILES}/https/ca/root-ca.crt.pem    ${SUPPORT_FILES}/https/ca/root-ca.crt
    Install System Ca Cert  ${SUPPORT_FILES}/https/ca/root-ca.crt

Set Bad Environment Proxy
    Set Environment Variable  https_proxy  http://localhost:2/
    Set Environment Variable  http_proxy  http://localhost:2/

Clear Environment Proxy
    Remove Environment Variable  https_proxy
    Remove Environment Variable  http_proxy

*** Test Case ***
Thin Installer can install via Update Cache
    Require Uninstalled
    Get Thininstaller
    Setup Update Cache
    Can Curl Url    https://localhost:1236/sophos/customer
    Set Local CA Environment Variable
    Regenerate Certificates
    Start Local Cloud Server
    Create Default Credentials File  update_caches=localhost:1236,2,1;localhost:1235,1,1
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller  0  no_connection_address_override=${True}
    Check Thininstaller Log Contains In Order
        ...  Listing warehouse with creds [9539d7d1f36a71bbac1259db9e868231] at [https://localhost:1235/sophos/customer]
        ...  Listing warehouse with creds [9539d7d1f36a71bbac1259db9e868231] at [https://localhost:1236/sophos/customer]
    Check Thininstaller Log does not contain  http://dci.sophosupd.com/update

Thin Installer can install via Update Cache With Bad Proxy
    Require Uninstalled
    Get Thininstaller
    Setup Update Cache
    Can Curl Url    https://localhost:1236/sophos/customer
    Set Local CA Environment Variable
    Set Bad Environment Proxy
    Regenerate Certificates
    Start Local Cloud Server
    Create Default Credentials File  update_caches=localhost:1236,2,1;localhost:1235,1,1
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller  0  no_connection_address_override=${True}
    Check Thininstaller Log Contains In Order
        ...  Listing warehouse with creds [9539d7d1f36a71bbac1259db9e868231] at [https://localhost:1235/sophos/customer]
        ...  Listing warehouse with creds [9539d7d1f36a71bbac1259db9e868231] at [https://localhost:1236/sophos/customer]
    Check Thininstaller Log does not contain  http://dci.sophosupd.com/update
    Check Thininstaller Log does not contain  http://localhost:2/

Thin Installer fails with bad update cache cert
    Require Uninstalled
    Get Thininstaller
    Create Default Credentials File  update_caches=localhost:1236,2,1;localhost:1235,1,1
    Build Default Creds Thininstaller From Sections
    Regenerate HTTPS Certificates
    Setup Update Cache
    Can Curl Url    https://localhost:1236/sophos/customer
    Set Local CA Environment Variable
    Regenerate Certificates
    Start Local Cloud Server
    Run Default Thininstaller  10    no_connection_address_override=${True}
    Check Thininstaller Log Contains In Order
        ...  Listing warehouse with creds [9539d7d1f36a71bbac1259db9e868231] at [https://localhost:1235/sophos/customer]
        ...  Listing warehouse with creds [9539d7d1f36a71bbac1259db9e868231] at [https://localhost:1236/sophos/customer]
        ...  Listing warehouse with creds [9539d7d1f36a71bbac1259db9e868231] at [http://dci.sophosupd.com/update]