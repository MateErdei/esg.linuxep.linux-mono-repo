*** Settings ***
Resource  McsRouterResources.robot

Suite Setup      Regenerate Certificates
Suite Teardown   Cleanup Certificates

Test Teardown     MCSRouter Default Test Teardown