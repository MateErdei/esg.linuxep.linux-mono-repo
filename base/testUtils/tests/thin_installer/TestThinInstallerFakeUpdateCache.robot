*** Settings ***

Library     ${COMMON_TEST_LIBS}/UpdateServer.py
Library     ${COMMON_TEST_LIBS}/ThinInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/OSUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/MCSRouter.py
Library     ${COMMON_TEST_LIBS}/FakeSDDS3UpdateCacheUtils.py

Library     Process
Library     OperatingSystem

Resource    ${COMMON_TEST_ROBOT}/GeneralTeardownResource.robot
Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/ThinInstallerResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Test Setup      Setup Thininstaller Test
Test Teardown   Teardown

Suite Setup  sdds3 suite setup with fakewarehouse with real base
Suite Teardown   SDDS3 Suite Fake Warehouse Teardown

Force Tags  THIN_INSTALLER  UPDATE_CACHE

*** Keywords ***
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

Thin Installer can install Directly and Fallback from broken update cache
    Create Default Credentials File  update_caches=localhost:1235,1,1
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller  0  force_certs_dir=${SDDS3_DEVCERTS}
    Check Suldownloader Log Contains In Order
        ...  Trying update via update cache: https://localhost:1235
        ...  Connecting to update source directly
        ...  Update success

Thin Installer can install via Update Cache and Fallback from broken update cache
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
    Create Default Credentials File  update_caches=localhost:8080,2,1;localhost:1235,1,1
    Build Default Creds Thininstaller From Sections
    Run Default Thininstaller  0  force_certs_dir=${SDDS3_DEVCERTS}
    Check Suldownloader Log Contains In Order
        ...  Trying update via update cache: https://localhost:1235
        ...  Trying update via update cache: https://localhost:8080
        ...  Update success

    check_suldownloader_log_should_not_contain  Connecting to update source directly
    check_suldownloader_log_should_not_contain  localhost:2

Thin Installer Respects Update Cache Override Set to None
    create_default_credentials_file  update_caches=localhost:8080,2,1;localhost:1235,1,1
    build_default_creds_thininstaller_from_sections
    run_default_thininstaller_with_args    ${0}    --update-caches=none    force_certs_dir=${SDDS3_DEVCERTS}

    check_thininstaller_log_contains    Update cache manually set to none, updates will not be performed via an update cache
    check_thininstaller_log_does_not_contain    List of update caches to install from: localhost:8080,2,1;localhost:1235,1,1

    check_suldownloader_log_should_not_contain    Trying update via update cache: https://localhost:1235
    check_suldownloader_log_should_not_contain    Trying update via update cache: https://localhost:8080

    check_suldownloader_log_contains    Connecting to update source directly
    check_suldownloader_log_contains    Update success

Thin Installer can Install via Update Cache Overriden by Argument
    create_default_credentials_file  update_caches=localhost:1235,1,1
    build_default_creds_thininstaller_from_sections

    run_default_thininstaller_with_args    ${0}    --update-caches=localhost:8080    force_certs_dir=${SDDS3_DEVCERTS}
    check_thininstaller_log_contains    List of update caches to install from: localhost:8080,0,overridden-update-cache

    check_suldownloader_log_contains_in_order
    ...  Trying update via update cache: https://localhost:8080
    ...  Update success

    check_suldownloader_log_should_not_contain    Connecting to update source directly
    check_suldownloader_log_should_not_contain    Trying update via update cache: https://localhost:1235


