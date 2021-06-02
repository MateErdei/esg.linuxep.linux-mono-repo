*** Settings ***
Documentation  Thin Installer tests which require a fake warehouse serving everest-base

# Thin installer can take a little while to run, e.g. 4 mins, which is close to the default test timeout of 5 mins
# so make it longer to allow for this.
Test Timeout    15 minutes

Test Setup   SetupServers
Test Teardown    Teardown

Suite Setup   Local Suite Setup
Suite Teardown   Local Suite Teardown

Library     ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library     ${LIBS_DIRECTORY}/WarehouseUtils.py
Library     ${LIBS_DIRECTORY}/UpdateServer.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py
Library     ${LIBS_DIRECTORY}/CentralUtils.py
Library     Process
Library     OperatingSystem
Library     String
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../installer/InstallerResources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    ThinInstallerResources.robot

Default Tags  THIN_INSTALLER  OSTIA
*** Variables ***
${PROXY_LOG}  ./tmp/proxy_server.log
${MCS_CONFIG_FILE}  ${SOPHOS_INSTALL}/base/etc/mcs.config
${CUSTOM_DIR_BASE}  /CustomPath
${BaseVUTPolicy}   ${GeneratedWarehousePolicies}/base_only_VUT.xml
${BASE_VUT_POLICY_NEVER_BALLISTA}   ${GeneratedWarehousePolicies}/base_only_VUT_no_ballista_override.xml

*** Keywords ***
Local Suite Setup
    Regenerate Certificates
    Copy File  ${SUPPORT_FILES}/CloudAutomation/root-ca.crt.pem  /tmp/root-ca.crt.pem
    Setup Ostia Warehouse Environment

Local Suite Teardown
    Teardown Ostia Warehouse Environment
    Remove File  /tmp/root-ca.crt.pem

SetupServers
    Ensure Fake Cloud Certs Generated
    Set Environment Variable  CORRUPTINSTALL  no
    Require Uninstalled
    Uninstall SAV
    Get Thininstaller
    Set Local CA Environment Variable
    Start Local Cloud Server  --initial-alc-policy  ${BaseVUTPolicy}

Teardown
    Run Keyword If Test Failed  Dump Thininstaller Log
    Log File  ${PROXY_LOG}
    General Test Teardown
    Stop Update Server
    Stop Proxy Servers
    Stop Proxy If Running
    Stop Local Cloud Server
    Run Keyword If Test Failed   Dump Cloud Server Log
    Cleanup Local Cloud Server Logs
    Teardown Reset Original Path
    Run Keyword If Test Failed    Dump Thininstaller Log
    Run Keyword And Ignore Error  Move File  /etc/hosts.bk  /etc/hosts
    Cleanup Files
    Require Uninstalled
    Reset Sophos Install Environment Variable
    Remove Directory  ${CUSTOM_DIR_BASE}  recursive=$true
    Cleanup Temporary Folders
    Cleanup Systemd Files

#Setup With Large Group Creation
#    Setup Group File With Large Group Creation
#    SetupServers
#Teardown With Large Group Creation
#    Teardown Group File With Large Group Creation
#    Teardown

Check Proxy Log Contains
    [Arguments]  ${pattern}  ${fail_message}
    ${ret} =  Grep File  ${PROXY_LOG}  ${pattern}
    Should Contain  ${ret}  ${pattern}  ${fail_message}

Check MCS Config Contains
    [Arguments]  ${pattern}  ${fail_message}
    ${ret} =  Grep File  ${MCS_CONFIG_FILE}  ${pattern}
    Should Contain  ${ret}  ${pattern}  ${fail_message}

Find IP Address With Distance
    [Arguments]  ${dist}
    ${result} =  Run Process  ip  addr
    ${ipaddresses} =  Get Regexp Matches  ${result.stdout}  inet (10[^/]*)  1
    ${head} =  Get Regexp Matches  ${ipaddresses[0]}  (10\.[^\.]*\.[^\.]*\.)[^\.]*  1
    ${tail} =  Get Regexp Matches  ${ipaddresses[0]}  10\.[^\.]*\.[^\.]*\.([^\.]*)  1
    ${tail_xored} =  Evaluate  ${tail[0]} ^ (2 ** ${dist})
    [return]  ${head[0]}${tail_xored}

Check All Sophos-spl Services Running
    Check Watchdog Running
    Check Management Agent Running
    Check Update Scheduler Running
    Check MCS Router Process Running

Check For Single Process Running
    [Arguments]  ${ProcessName}
    ${result}=   Run Process  ps  -ef
    ${lines}=  Get Lines Matching Pattern  ${result.stdout}  *${ProcessName}*
    Should Contain X Times  ${lines}  ${ProcessName}  1  Zero or more than one process with name: ${ProcessName}


Check All Relevant Logs Contain Install Path
    [Arguments]   ${Install_Path}
    Check Log Contains  ${Install_Path}  ${Install_Path}/sophos-spl/logs/base/register_central.log  Register Central
    Check Log Contains  ${Install_Path}  ${Install_Path}/sophos-spl/logs/base/watchdog.log  Watchdog
    Check Log Contains  ${Install_Path}  ${Install_Path}/sophos-spl/logs/base/wdctl.log  WDCTL
    Check Log Contains  ${Install_Path}  ${Install_Path}/sophos-spl/logs/base/sophosspl/sophos_managementagent.log  Management Agent
    Check Log Contains  ${Install_Path}  ${Install_Path}/sophos-spl/logs/base/sophosspl/mcsrouter.log  MCS Router

*** Test Case ***

#Thin Installer Environment Proxy
#    Should Not Exist    ${SOPHOS_INSTALL}
#    Check MCS Router Not Running
#    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
#    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation
#
#    Start Simple Proxy Server  10000
#    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  proxy=http://localhost:10000
#
#    ${customer_file_address} =  Get Customer File Domain For Policy  ${BaseVUTPolicy}
#    ${warehouse_address} =  Get Warehouse Domain For Policy  ${BaseVUTPolicy}
#
#    Check Proxy Log Contains  "CONNECT localhost:4443 HTTP/1.1" 200  Proxy Log does not show connection to Fake Cloud
#    Check Proxy Log Contains  "CONNECT ${customer_file_address} HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
#    Check Proxy Log Contains  "CONNECT ${warehouse_address} HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
#    Check MCS Config Contains  proxy=http://localhost:10000  MCS Config does not have proxy present
#
#    Check Thininstaller Log Does Not Contain  ERROR
#    Check Thininstaller Log Contains  DEBUG: Checking we can connect to Sophos Central (at https://localhost:4443/mcs via http://localhost:10000)\nDEBUG: Set CURLOPT_PROXYAUTH to CURLAUTH_ANY\nDEBUG: Set CURLOPT_PROXY to: http://localhost:10000\nDEBUG: Successfully got [No error] from Sophos Central
#    Check Root Directory Permissions Are Not Changed

Thin Installer Digest Proxy
    Should Not Exist    ${SOPHOS_INSTALL}
    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation

    Start Proxy Server With Digest Auth  10000  username  password
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  proxy=http://username:password@localhost:10000

    ${customer_file_address} =  Get Customer File Domain For Policy  ${BaseVUTPolicy}
    ${warehouse_address} =  Get Warehouse Domain For Policy  ${BaseVUTPolicy}

    Check Proxy Log Contains  "CONNECT localhost:4443 HTTP/1.1" 200  Proxy Log does not show connection to Fake Cloud
    Check Proxy Log Contains  "CONNECT ${customer_file_address} HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check Proxy Log Contains  "CONNECT ${warehouse_address} HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check MCS Config Contains  proxy=http://username:password@localhost:10000  MCS Config does not have proxy present

    Check Thininstaller Log Does Not Contain  ERROR

    Check Thininstaller Log Contains  DEBUG: Checking we can connect to Sophos Central (at https://localhost:4443/mcs via http://username:password@localhost:10000)\nDEBUG: Set CURLOPT_PROXYAUTH to CURLAUTH_ANY\nDEBUG: Set CURLOPT_PROXY to: http://username:password@localhost:10000\nDEBUG: Successfully got [No error] from Sophos Central
    Check Root Directory Permissions Are Not Changed

Thin Installer Basic Proxy
    Should Not Exist    ${SOPHOS_INSTALL}
    Check MCS Router Not Running
    ${result} =  Run Process    pgrep  -f  ${MANAGEMENT_AGENT}
    Should Not Be Equal As Integers  ${result.rc}  0  Management Agent running before installation

    Start Proxy Server With Basic Auth  10000  username  password
    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseVUTPolicy}  proxy=http://username:password@localhost:10000

    ${customer_file_address} =  Get Customer File Domain For Policy  ${BaseVUTPolicy}
    ${warehouse_address} =  Get Warehouse Domain For Policy  ${BaseVUTPolicy}

    Check Proxy Log Contains  "CONNECT localhost:4443 HTTP/1.1" 200  Proxy Log does not show connection to Fake Cloud
    Check Proxy Log Contains  "CONNECT ${customer_file_address} HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check Proxy Log Contains  "CONNECT ${warehouse_address} HTTP/1.1" 200  Proxy Log does not show connection to Fake Warehouse
    Check MCS Config Contains  proxy=http://username:password@localhost:10000  MCS Config does not have proxy present

    Check Thininstaller Log Does Not Contain  ERROR
    Check Thininstaller Log Contains  DEBUG: Checking we can connect to Sophos Central (at https://localhost:4443/mcs via http://username:password@localhost:10000)\nDEBUG: Set CURLOPT_PROXYAUTH to CURLAUTH_ANY\nDEBUG: Set CURLOPT_PROXY to: http://username:password@localhost:10000\nDEBUG: Successfully got [No error] from Sophos Central
    Check Root Directory Permissions Are Not Changed

