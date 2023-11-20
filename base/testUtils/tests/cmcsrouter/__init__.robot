*** Settings ***
Resource  ${COMMON_TEST_ROBOT}/McsRouterResources.robot

Suite Setup      Regenerate Certificates

Test Teardown     MCSRouter Default Test Teardown