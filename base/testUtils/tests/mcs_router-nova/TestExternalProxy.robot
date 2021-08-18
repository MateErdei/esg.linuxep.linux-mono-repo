*** Settings ***
# This test uses the qa central account LinuxDarwinSSPLAutomation@mail.com
Suite Setup       Set Suite Variable    ${regCommand}     /opt/sophos-spl/base/bin/registerCentral 8f4b3234643165725ca6a9a2b87a0893880f4041ff7d2887a89d74ca329af4ce https://mcs2-cloudstation-eu-central-1.qa.hydra.sophos.com/sophos/management/ep   children=true

Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/ProxyUtils.py
Resource  ../mcs_router/McsRouterResources.robot

Resource    ../GeneralTeardownResource.robot
Resource  ../installer/InstallerResources.robot

Test Teardown   Cleanup QA account Test
Test Setup  Setup QA account Test

Default Tags  CENTRAL  MCS   EXCLUDE_AWS
*** Test Cases ***
Test we can reach central through an enviroment proxy
    [Timeout]  10 minutes
    Require Fresh Install
    Create File  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Check Expected Base Processes Are Running
    Set Environment Variable  https_proxy   http://ssplsecureproxyserver.eng.sophos:8888
    Register With Central  ${regCommand}
    Wait Until Keyword Succeeds
    ...  40 seconds
    ...  5 secs
    ...  Check Register Central Log Contains  Successfully connected to mcs2-cloudstation-eu-central-1.qa.hydra.sophos.com:443 via ssplsecureproxyserver.eng.sophos:8888
    Wait For MCS Router To Be Running
    Wait Until Keyword Succeeds
    ...  40 seconds
    ...  5 secs
    ...  Check Mcsrouter Log Contains  Successfully connected to mcs2-cloudstation-eu-central-1.qa.hydra.sophos.com:443 via ssplsecureproxyserver.eng.sophos:8888
    Wait Until Keyword Succeeds
    ...  60 seconds
    ...  10 secs
    ...  Check Policies Exist

*** Keywords ***
Setup QA account Test
    Set Environment Variable  MCS_CA   /tmp/hmr-qa-sha256.pem
    Copy File  ${SUPPORT_FILES}/CloudAutomation/hmr-qa-sha256.pem  /tmp/hmr-qa-sha256.pem
    Run Process  chmod  +xr  /tmp/hmr-qa-sha256.pem
#    Check Secure Proxy Is Up
    # ensure the secureproxy is in a good state before tests
    ${Result}=  wait For Secure Proxy Server To Be Up
    Run Keyword If   ${Result}==${False}  Restart Secure Server Proxy

Cleanup QA account Test
    Remove Environment Variable  https_proxy
    Remove Environment Variable  MCS_CA
    MCSRouter Test Teardown
    Deregister From Central
    Require Uninstalled



