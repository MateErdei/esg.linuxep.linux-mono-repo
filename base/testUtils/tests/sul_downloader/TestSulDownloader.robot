*** Settings ***
Documentation     Test the SUL Downloader

Library           Process
Library           OperatingSystem
Library           Collections

Library           ${LIBS_DIRECTORY}/SulDownloader.py
Library           ${LIBS_DIRECTORY}/UpdateServer.py
Library           ${LIBS_DIRECTORY}/WarehouseGenerator.py
Library           ${LIBS_DIRECTORY}/LogUtils.py
Library           ${LIBS_DIRECTORY}/OSUtils.py

Resource          SulDownloaderResources.robot
Resource          ../installer/InstallerResources.robot
Resource          ../GeneralTeardownResource.robot
Resource          ../upgrade_product/UpgradeResources.robot

Suite Setup       Generate Update Certs
Suite Teardown    Terminate All Processes    kill=True

Test Setup        Setup For Test
Test Teardown     Teardown for Test

Default Tags  SULDOWNLOADER

*** Keywords ***

Recreate Installation In Temp Dir
    Remove Directory    ${tmpdir}/sspl    recursive=True
    Create Directory    ${tmpdir}/sspl/tmp
    Create Directory    ${tmpdir}/var/lock
    Create Directory    ${tmpdir}/tmp
    Create Directory    ${tmpdir}/sspl/base/update
    Should Exist        ${SOPHOS_INSTALL}
    Create File         ${SOPHOS_INSTALL}/base/etc/logger.conf.0   [suldownloader]\nVERBOSITY=DEBUG\n
    Run Process         ln  -snf  ${VERSIGPATH}   ${tmpdir}/sspl/base/update/versig
    Create Directory    ${tmpdir}/sspl/base/update/cache/primarywarehouse
    Create Directory    ${tmpdir}/sspl/base/update/cache/primary
    Create Directory    ${tmpdir}/sspl/base/update/cache/primary
    Create Directory    ${InstallProductsDir}

Setup For Test
    Recreate Installation In Temp Dir
    Copy file  ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt   ${SOPHOS_INSTALL}/base/update/rootcerts/
    Copy file  ${SUPPORT_FILES}/sophos_certs/rootca.crt   ${SOPHOS_INSTALL}/base/update/rootcerts/
    Install Local SSL Server Cert To System
    Set Environment Variable  CORRUPTINSTALL  no

Teardown for Test
    General Test Teardown
    Run Keyword If Test Failed  LogUtils.Dump Log  ./tmp/proxy_server.log
    Run Keyword If Test Failed  LogUtils.Dump Log  ${tmpdir}/sspl/logs/base/suldownloader.log
    Remove Environment Variable  http_proxy
    Remove Environment Variable  https_proxy
    Run Keyword If Test Failed  Display All tmp Files Present

    Stop Update Server
    Stop Proxy Servers
    Revert System CA Certs
    Variable Should Exist    ${tmpdir}
    Remove Directory   ${tmpdir}    recursive=True
    Set Environment Variable  CORRUPTINSTALL  no

    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log

*** Variables ***

${SUCCESS}                      0
${SULDOWNLOADERLOCKFILEERROR}   11
${INSTALLFAILED}                103
${DOWNLOADFAILED}               107
${RESTARTNEEDED}                109
${PACKAGESOURCEMISSING}         111
${CONNECTIONERROR}              112
${UNINSTALLFAILED}              120
${UNSPECIFIED}                  121
${FILEREADORWRITEERROR}         254
${INVALIDCOMMANDLINEARGUMENTS}  255

${tmpdir}                       ${SOPHOS_INSTALL}/tmp/SDT
${InstallProductsDir}           ${tmpdir}/sspl/base/update/var/installedproducts

*** Test Cases ***

No arguments
    [Documentation]    SUL Downloader expects two arguments, ensure that we get an error if we give no args
    ${result} =    Run Process    ${SUL_DOWNLOADER}
    Check SulDownloader Result   ${result}   ${INVALIDCOMMANDLINEARGUMENTS}
    ...    Error, invalid command line arguments
    ...    Usage: SULDownloader inputpath outputpath

Suldownloader Log Has Correct Permissions
    [Documentation]    Sul Downloader log  is root owner and 0600
    ${result} =    Run Process    ${SUL_DOWNLOADER}
    File Exists With Permissions  ${SOPHOS_INSTALL}/logs/base/suldownloader.log  root  root  -rw-------


Single argument
    [Documentation]    SUL Downloader expects two arguments, ensure that we get an error if we give just one arg
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/input
    Check SulDownloader Result   ${result}   ${INVALIDCOMMANDLINEARGUMENTS}
    ...    Error, invalid command line arguments
    ...    Usage: SULDownloader inputpath outputpath

#Three arguments
#    [Documentation]    SUL Downloader expects two arguments, ensure that we get an error if we give too many args
#    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/input    ${tmpdir}/output    extra_argument
#    Log    "stdout = ${result.stdout}"
#    Log    "stderr = ${result.stderr}"
#    Error Codes Match   ${result.rc}    ${INVALIDCOMMANDLINEARGUMENTS}
#    Should Contain    ${result.stderr}    Error, invalid command line arguments
#    Should Contain    ${result.stderr}    Usage: SULDownloader inputpath outputpath

Input doesn't exist
    [Documentation]    Specify an input file that doesn't exist. Check that an error is reported.
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/no_input    ${tmpdir}/output
    Check SulDownloader Result   ${result}   ${FILEREADORWRITEERROR}
    ...    Failed to read file
    ...    file does not exist

Input is directory
    [Documentation]    Specify an input file that is actually a directory. Check that an error is reported.
    Create Directory    ${tmpdir}/input
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/input    ${tmpdir}/output

    Check SulDownloader Result   ${result}   ${FILEREADORWRITEERROR}
    ...    Failed to read file
    ...    is a directory


Output doesn't exist
    [Documentation]    Specify an output path that doesn't exist. Check that an error is reported.
    ${result} =    Run Process    ${SUL_DOWNLOADER}    /dev/null    ${tmpdir}/no_dir/output
    Check SulDownloader Result   ${result}   ${FILEREADORWRITEERROR}
    ...    The directory of the output path does not exists
    ...    The directory to write the output file must exist


Output is directory
    [Documentation]    Specify an output path that is a directory. Check that an error is reported.
    Create Directory    ${tmpdir}/output
    ${result} =    Run Process    ${SUL_DOWNLOADER}    /dev/null    ${tmpdir}/output
    Check SulDownloader Result   ${result}   ${FILEREADORWRITEERROR}
    ...    Output path given is directory
    ...    Output file path cannot be a directory


Empty input file
    [Documentation]    Provide an input file that is empty. Check that an error is reported.
    Create File    ${tmpdir}/input
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/input    ${tmpdir}/output
    Check SulDownloader Result   ${result}   ${UNSPECIFIED}
    ...    Failed to process input settings
    ...    Failed to process json message

Unreachable warehouse server
    Create Directory    ${tmpdir}/sspl/base/update/cache/primarywarehouse
    Create Directory    ${tmpdir}/sspl/base/update/cache/primary
    ${config} =    Create JSON Config    install_path=${tmpdir}/sspl
    Create File    ${tmpdir}/update_config.json    content=${config}
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/output

    Check SulDownloader Result   ${result}   ${CONNECTIONERROR}
    ...    Failed to connect to the warehouse

    Check SulDownloader Log Contains
    ...    Cannot locate server for

    Check SulDownloader Log Should Not Contain
    ...    SSL connection errors


Empty warehouse
    Create Directory    ${tmpdir}/sspl/base/update/cache/primarywarehouse
    Create Directory    ${tmpdir}/sspl/base/update/cache/primary

    Create Directory    ${tmpdir}/warehouse
    Start Update Server    1233    ${tmpdir}/warehouse
    Can Curl Url    https://localhost:1233/

    ${config} =    Create JSON Config    install_path=${tmpdir}/sspl
    Create File    ${tmpdir}/update_config.json    content=${config}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${CONNECTIONERROR}
    ...    Failed to connect to the warehouse

    Check SulDownloader Log Contains
    ...    Couldn't find DCI for user

    Check SulDownloader Log Should Not Contain
    ...    SSL connection errors


Wrong Signature warehouse
    Set Environment Variable  CORRUPTINSTALL  yes
    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${INSTALLFAILED}

    Check SulDownloader Report Contains
    ...     failed signature verification


Test warehouse
    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

Installer fails
    ${result} =  Perform Install  1  INSTALLER EXECUTED - FAILING  ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${INSTALLFAILED}

    Check SulDownloader Report Contains
    ...     INSTALLFAILED


Invalid creds
    Require Warehouse With Fake Single Installer Product

    ${config} =    Create Config
    Set to Dictionary    ${config["credential"]}    username=invalid
    ${config_json} =    Data to JSON    ${config}
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${CONNECTIONERROR}
    ...     Failed to connect to the warehouse

    Check SulDownloader Log Contains
    ...     Couldn't find DCI for user


Simple proxy
    [Tags]  SMOKE  SULDOWNLOADER
    Require Warehouse With Fake Single Installer Product

    Start Simple Proxy Server    1235

    ${config} =    Create Config
    ${proxy_url} =    Set Variable    http://localhost:1235/
    Set to Dictionary    ${config["proxy"]}    url=${proxy_url}
    Log  ${Config}
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    log  ${tmpdir}
    should exist  /opt/sophos-spl/tmp
    should exist  /opt/sophos-spl/tmp/SDT
#    log to console   sleeping before sul runs
#    sleep  300
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json  env:SOPHOS_INSTALL=${SOPHOS_INSTALL}

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    Verify SulDownloader Connects Via Proxy   ${proxy_url}

Simple proxy Accepts User Not setting http prefix
    Require Warehouse With Fake Single Installer Product

    Start Simple Proxy Server    1235

    ${config} =    Create Config
    ${proxy_url} =    Set Variable    http://localhost:1235/
    ${proxy_url_set_by_user} =    Set Variable    localhost:1235
    Set to Dictionary    ${config["proxy"]}    url=${proxy_url_set_by_user}
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    Verify SulDownloader Connects Via Proxy   ${proxy_url_set_by_user}


SulDownloader Should not claim proxy connection when sul will not apply it
    # sul let to curl to apply the environment proxy. And curl applies https_proxy if the connection is to https site
    # https://curl.haxx.se/libcurl/c/libcurl-env.html
    Require Warehouse With Fake Single Installer Product

    Start Simple Proxy Server    1235

    ${config} =    Create Config
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    ${proxy_url} =    Set Variable    http://localhost:1235/
    Set Environment Variable  http_proxy   ${proxy_url}
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    Check SulDownloader Log Should Not Contain  Successfully connected to: Sophos at https://localhost:1233 \ via environment proxy
    Run Keyword and Expect Error  *does not contain*   Check Proxy Log Contains     CONNECT localhost:1233




Simple environment proxy
    Require Warehouse With Fake Single Installer Product

    Start Simple Proxy Server    1235

    ${config} =    Create Config
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    ${proxy_url} =    Set Variable    http://localhost:1235/
    Set Environment Variable  https_proxy   ${proxy_url}
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    Verify SulDownloader Connects Via Environment Proxy
    Check Proxy Log Contains   CONNECT localhost:1233
    Check Proxy Log Contains   CONNECT localhost:1234


Wrong environment proxy
    Require Warehouse With Fake Single Installer Product

    Start Simple Proxy Server    1235

    ${config} =    Create Config
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}
    # wrong por number. It should be 1235
    ${proxy_url} =    Set Variable    http://localhost:1237/
    Set Environment Variable  https_proxy   ${proxy_url}
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED
    Run Keyword and Expect Error  *does not contain*   Check Proxy Log Contains     CONNECT localhost:1233
    Check SulDownloader Log Should Not Contain  Successfully connected to: Sophos at https://localhost:1233 \ via environment proxy


Update through basic auth proxy unobfuscated creds
    Require Warehouse With Fake Single Installer Product

    ${username} =  Set Variable  username
    ${password} =  Set Variable  password
    Start Proxy Server With Basic Auth    1235  ${username}  ${password}

    ${config} =    Create Config
    ${proxy_url} =    Set Variable    http://localhost:1235/
    Set to Dictionary    ${config["proxy"]}    url=${proxy_url}
    Set to Dictionary    ${config["proxy"]["credential"]}  username=${username}  password=${password}  proxyType=1
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    Verify SulDownloader Connects Via Proxy   ${proxy_url}


Update through basic auth proxy unobfuscated creds with obfuscated password
    Require Warehouse With Fake Single Installer Product

    ${username} =  Set Variable  username
    ${password} =  Set Variable  password
    ${obfuscated} =  Set Variable  CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=
    Start Proxy Server With Basic Auth    1235  ${username}  ${password}

    ${config} =    Create Config
    ${proxy_url} =    Set Variable    http://localhost:1235/
    Set to Dictionary    ${config["proxy"]}    url=${proxy_url}
    Set to Dictionary    ${config["proxy"]["credential"]}  username=${username}  password=${obfuscated}  proxyType=2
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    Verify SulDownloader Connects Via Proxy   ${proxy_url}


When Update through auth proxy fails it does not log plain password

    Require Warehouse With Fake Single Installer Product

    ${username} =  Set Variable  username
    ${password} =  Set Variable  different
    # the obfuscated refers to password
    ${obfuscated} =  Set Variable  CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=
    Start Proxy Server With Basic Auth    1235  ${username}  ${password}

    ${config} =    Create Config
    ${proxy_url} =    Set Variable    http://localhost:1235/
    Set to Dictionary    ${config["proxy"]}    url=${proxy_url}
    Set to Dictionary    ${config["proxy"]["credential"]}  username=${username}  password=${obfuscated}  proxyType=2
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    Check SulDownloader Log Should Not Contain  Successfully connected to: Sophos at https://localhost:1233 via proxy: http://localhost:1235
    Check SulDownloader Log Should Not Contain  user:password


Simple proxy with obfuscated password as Sent By Central
    Require Warehouse With Fake Single Installer Product

    # is equivalent to empty
    ${obfuscated} =  Set Variable  CCDN+JdsRVNd+yKFqQhrmdJ856KCCLHLQxEtgwG/tD5myvTrUk/kuALeUDhL4plxGvM=
    Start Simple Proxy Server    1235

    ${config} =    Create Config
    ${proxy_url} =    Set Variable    http://localhost:1235/
    Set to Dictionary    ${config["proxy"]}    url=${proxy_url}
    Set to Dictionary    ${config["proxy"]["credential"]}  password=${obfuscated}  proxyType=2
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    Verify SulDownloader Connects Via Proxy   ${proxy_url}


Update through digest auth proxy unobfuscated creds
    Require Warehouse With Fake Single Installer Product
    ${username} =  Set Variable  username
    ${password} =  Set Variable  password
    Start Proxy Server With Digest Auth    1235  ${username}  ${password}

    ${config} =    Create Config
    ${proxy_url} =    Set Variable    http://localhost:1235/
    Set to Dictionary    ${config["proxy"]}    url=${proxy_url}
    Set to Dictionary    ${config["proxy"]["credential"]}  username=${username}  password=${password}  proxyType=1
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    Verify SulDownloader Connects Via Proxy   ${proxy_url}


Update through basic auth proxy obfuscated creds
    Require Warehouse With Fake Single Installer Product

    ${username} =  Set Variable  username
    ${password} =  Set Variable  password
    Start Proxy Server With Basic Auth    1235  ${username}  ${password}

    ${config} =    Create Config
    ${proxy_url} =    Set Variable    http://localhost:1235/
    ${obfuscatedpassword} =    Set Variable    CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=
    Set to Dictionary    ${config["proxy"]}    url=${proxy_url}
    Set to Dictionary    ${config["proxy"]["credential"]}  username=${username}  password=${obfuscatedpassword}  proxyType=2
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    Verify SulDownloader Connects Via Proxy   ${proxy_url}

Update through digest auth proxy obfuscated creds
    Require Warehouse With Fake Single Installer Product

    ${username} =  Set Variable  username
    ${password} =  Set Variable  password
    Start Proxy Server With Digest Auth    1235  ${username}  ${password}

    ${config} =    Create Config
    ${proxy_url} =    Set Variable    http://localhost:1235/
    ${obfuscatedpassword} =    Set Variable    CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=
    Set to Dictionary    ${config["proxy"]}    url=${proxy_url}
    Set to Dictionary    ${config["proxy"]["credential"]}  username=${username}  password=${obfuscatedpassword}  proxyType=2
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    Verify SulDownloader Connects Via Proxy   ${proxy_url}


Unreachable proxy
    Require Warehouse With Fake Single Installer Product

    ${config} =    Create Config
    ${proxy_url} =    Set Variable    http://localhost:1235/
    Set to Dictionary    ${config["proxy"]}    url=${proxy_url}
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Stop Proxy Servers

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    # Following three message show that we failed to connect through a proxy and so went direct and suceeded.
    ${Fail_Connect_Message} =  Set Variable  Failed to connect to: Sophos at https://localhost:1233 via proxy: http://localhost:1235/
    ${Succeed_To_Download_Directly} =  Set Variable  Successfully connected to: Sophos at https://localhost:1233

    Check Suldownloader Log Contains In Order
    ...  ${Fail_Connect_Message}
    ...  ${Succeed_To_Download_Directly}


Can Use Authenticated Proxy Saved In Savedproxy Config
    [Tags]  SMOKE  SULDOWNLOADER
    Require Warehouse With Fake Single Installer Product

    ${username} =  Set Variable  username
    ${password} =  Set Variable  password
    Start Proxy Server With Basic Auth    1235  ${username}  ${password}

    ${config} =    Create Config
    ${proxy_domain_and_port} =  Set Variable   localhost:1235
    ${username_password} =      Set Variable   ${username}:${password}
    ${proxy_url} =              Set Variable   http://${username_password}@${proxy_domain_and_port}

    Log  ${config}
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Log    "config_json = ${config_json}"
    Create File    ${tmpdir}/update_config.json    content=${config_json}

    Create File    ${tmpdir}/sspl/base/etc/savedproxy.config    content=${proxy_url}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check SulDownloader Result   ${result}   ${SUCCESS}

    Check SulDownloader Report Contains
    ...     SUCCESS
    ...     UPGRADED

    Verify SulDownloader Connects Via Proxy   ${proxy_domain_and_port}


Test Product Uninstalls If Not In Warehouse
    ${UninstallMessage} =   Set Variable   UNINSTALLER EXECUTED
    Create Uninstall File   0    ${UninstallMessage}   SSPL-RIGIDNAME-2
    Run Process    chmod +x ${InstallProductsDir}/SSPL-RIGIDNAME-2.sh     shell=True

    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UNINSTALLED
    Should Contain    ${output}    UPGRADED
    Should Contain    ${log_contents}    ${UninstallMessage}
    Should Contain    ${log_contents}    INSTALLER EXECUTED
    Should Match Regexp     ${log_contents}    INFO \\[\\d+\\] suldownloader <> Downloaded products: ServerProtectionLinux-Base
    Should Not Match Regexp     ${log_contents}    WARN \\[\\d+\\] suldownloader <> Downloaded products: ServerProtectionLinux-Base


Test Product Fails Uninstall If Execution Fails To Run
    ${UninstallMessage} =   Set Variable   UNINSTALLER EXECUTED
    Create Uninstall File   0    ${UninstallMessage}   SSPL-RIGIDNAME-2
    #No execute permission being set on the uninstall script
    #Run Process    chmod +x ${InstallProductsDir}/SSPL-RIGIDNAME-2.sh     shell=True

    ${result} =  Perform Install   0  INSTALLER EXECUTED

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Error Codes Match   ${result.rc}    ${UNINSTALLFAILED}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    UNINSTALLFAILED
    Should Not Contain    ${log_contents}    ${UninstallMessage}
    Should Not Contain    ${log_contents}    ${UninstallMessage}
    ## Installer executed output directly to stdout, not the log file
    Should Contain        ${log_contents}    INSTALLER EXECUTED


Test Product Fails Uninstall If Execution Fails To Successfully Complete
    ${UninstallMessage} =   Set Variable   UNINSTALLER EXECUTED AND FAILED
    Create Uninstall File   1    ${UninstallMessage}    SSPL-RIGIDNAME-2

    Run Process    chmod +x ${InstallProductsDir}/SSPL-RIGIDNAME-2.sh     shell=True

    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Error Codes Match   ${result.rc}    ${UNINSTALLFAILED}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    UNINSTALLFAILED
    Should Contain    ${log_contents}    ${UninstallMessage}
    Should Contain    ${log_contents}     INSTALLER EXECUTED


Test Product Does Not Uninstall If Product Is Downloaded And Installed From Warehouse
    ${UninstallMessage} =   Set Variable   UNINSTALLER EXECUTED AND FAILED
    Create Uninstall File   1    ${UninstallMessage}    SSPL-RIGIDNAME-2

    Run Process    chmod +x ${InstallProductsDir}/SSPL-RIGIDNAME-2.sh     shell=True

    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json   SSPL-RIGIDNAME-2  SSPL-RIGIDNAME-2

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match    ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UPGRADED
    Should Not Contain    ${output}    UNINSTALLED

    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log
    Should Not Contain    ${log_contents}    ${UninstallMessage}
    Should Contain    ${log_contents}   INSTALLER EXECUTED


Test Product Does Not Install On Second Download If Downloaded Twice And An Uninstall Will Still Run
    #generate install file in directory    ${tmpdir}/TestInstallFiles/
    ${UninstallMessage} =   Set Variable   UNINSTALLER-EXECUTED

    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match    ${result.rc}    ${SUCCESS}

    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log
    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Not Contain    ${output}    UNINSTALLED
    Should Contain    ${output}    UPGRADED
    Should Contain    ${log_contents}   INSTALLER EXECUTED
    Should Not Contain    ${log_contents}    ${UninstallMessage}

    Create Uninstall File   0    ${UninstallMessage}   SSPL-RIGIDNAME-2

    Run Process    chmod +x ${InstallProductsDir}/SSPL-RIGIDNAME-2.sh     shell=True

    Move File   ${tmpdir}/update_report.json   ${tmpdir}/update_report_2018_08_21_09_43_21.json
    Remove File  ${tmpdir}/sspl/logs/base/suldownloader.log

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json  env:SOPHOS_INSTALL=${SOPHOS_INSTALL}/tmp/SDT/sspl

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Error Codes Match    ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UNINSTALLED
    Should Contain    ${output}    UPTODATE
    Should Contain        ${log_contents}    ${UninstallMessage}
    Should Not Contain    ${log_contents}    INSTALLER EXECUTED


Test Product Should Force Reinstall After It Failed On The Install Even If Distribution Is The Same
    ${InstallSuccessFilePath} =   Set Variable  ${tmpdir}/InstallSucceed.txt
    #generate install file in directory    ${tmpdir}/TestInstallFiles/
    Create Success Or Fail Install File Depending On Existence Of File  INSTALLER EXECUTED  INSTALLER FAILED  ${InstallSuccessFilePath}

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}

	Generate Warehouse
    Start Update Server    1233    ${tmpdir}/temp_warehouse/customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1233
    Can Curl Url    https://localhost:1234/catalogue/sdds.${BASE_RIGID_NAME}.xml

    ${config} =    Create JSON Config    install_path=${tmpdir}/sspl
    Create File    ${tmpdir}/update_config.json    content=${config}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match    ${result.rc}    ${INSTALLFAILED}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    INSTALLFAILED
    Should Contain    ${log_contents}    INSTALLER FAILED

    Move File   ${tmpdir}/update_report.json   ${tmpdir}/update_report_2018_08_21_09_43_21.json
    Create File   ${InstallSuccessFilePath}
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match    ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UPGRADED
    Should Contain    ${log_contents}    INSTALLER EXECUTED


Test Product Should Force Reinstall After It Fails For Unspecified Reason

    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json  ${BASE_RIGID_NAME}  ${BASE_RIGID_NAME}  ${tmpdir}/NotAPath

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match    ${result.rc}    ${UNSPECIFIED}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    UNSPECIFIED
    #Report should not contain any products
    Should Not Contain    ${output}    ${BASE_RIGID_NAME}

    Move File   ${tmpdir}/update_report.json   ${tmpdir}/update_report_2018_08_21_09_43_21.json

    Stop Update Server
    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Error Codes Match    ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UPGRADED
    Should Contain    ${log_contents}    INSTALLER EXECUTED


Test Product Should Install After Package Source Missing Error

    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json  SSPL-DIFF-RIGIDNAME  ${BASE_RIGID_NAME}

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Error Codes Match    ${result.rc}    ${PACKAGESOURCEMISSING}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    PACKAGESOURCEMISSING
    #Report should not contain any products
    Should Not Contain    ${output}    products
    Should Contain    ${log_contents}    ${BASE_RIGID_NAME}

    Move File   ${tmpdir}/update_report.json   ${tmpdir}/update_report_2018_08_21_09_43_21.json

    Stop Update Server
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json  SSPL-DIFF-RIGIDNAME  SSPL-DIFF-RIGIDNAME

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Error Codes Match    ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SSPL-DIFF-RIGIDNAME
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UPGRADED

    Should Contain        ${log_contents}    INSTALLER EXECUTED
    Should Not Contain    ${log_contents}    ${BASE_RIGID_NAME}



Test Product Does Not Trigger A Reinstall After A Failed Uninstall
    ${UninstallMessage} =   Set Variable   UNINSTALLER-EXECUTED AND FAILED
    Create Uninstall File   1    ${UninstallMessage}   SSPL-RIGIDNAME-2
    Run Process    chmod +x ${InstallProductsDir}/SSPL-RIGIDNAME-2.sh     shell=True

    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Error Codes Match   ${result.rc}    ${UNINSTALLFAILED}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    UNINSTALLFAILED
    Should Contain    ${output}    UPGRADED
    Should Contain    ${log_contents}    ${UninstallMessage}
    Should Contain    ${log_contents}    INSTALLER EXECUTED
    Should Contain    ${log_contents}    UNINSTALLER-EXECUTED AND FAILED

    Move File   ${tmpdir}/update_report.json   ${tmpdir}/update_report_2018_08_21_09_43_21.json

    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Error Codes Match   ${result.rc}    ${UNINSTALLFAILED}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain        ${output}    UNINSTALLFAILED
    Should Not Contain    ${output}    UPGRADED
    Should Contain        ${output}    UPTODATE

    Should Contain        ${log_contents}    ${UninstallMessage}
    Should Not Contain    ${log_contents}    INSTALLER EXECUTED
    Should Not Contain    ${log_contents}    INSTALLER EXECUTED
    Should Contain        ${log_contents}    UNINSTALLER-EXECUTED AND FAILED

Test Product Does Trigger A Reinstall After A Failed Install And Failed Uninstall
    ${UninstallMessage} =   Set Variable   UNINSTALLER-EXECUTED AND FAILED
    Create Uninstall File   1    ${UninstallMessage}   SSPL-RIGIDNAME-2
    Run Process    chmod +x ${InstallProductsDir}/SSPL-RIGIDNAME-2.sh     shell=True

    ${InstallSuccessFilePath} =   Set Variable  ${tmpdir}/InstallSucceed.txt

    #generate install file in directory    ${tmpdir}/TestInstallFiles/
    Create Success Or Fail Install File Depending On Existence Of File  INSTALLER EXECUTED  INSTALLER FAILED  ${InstallSuccessFilePath}

    #Create Install File   1   INSTALLER FAILED  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}
    Log     ${BASE_RIGID_NAME}
    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}

	Generate Warehouse

    Start Update Server    1233    ${tmpdir}/temp_warehouse/customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1233
    Can Curl Url    https://localhost:1234/catalogue/sdds.${BASE_RIGID_NAME}.xml

    ${config} =    Create JSON Config    install_path=${tmpdir}/sspl
    Create File    ${tmpdir}/update_config.json    content=${config}

    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Log File  /opt/sophos-spl/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Error Codes Match    ${result.rc}    ${INSTALLFAILED}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    INSTALLFAILED
    Should Contain    ${output}    UNINSTALLFAILED
    Should Contain    ${log_contents}    INSTALLER FAILED
    Should Contain    ${log_contents}    UNINSTALLER-EXECUTED AND FAILED

    Move File   ${tmpdir}/update_report.json   ${tmpdir}/update_report_2018_08_21_09_43_21.json
    Create File   ${InstallSuccessFilePath}

    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Error Codes Match    ${result.rc}    ${UNINSTALLFAILED}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    UNINSTALLFAILED
    Should Contain    ${output}    UPGRADED
    Should Contain    ${log_contents}    INSTALLER EXECUTED
    Should Contain    ${log_contents}    UNINSTALLER-EXECUTED AND FAILED


Test Product Sul Distribution Error Will Force Reinstall On Next Update
    # Sul Distribution Error will report DownloadFailed.

    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UPGRADED
    Should Contain    ${log_contents}    INSTALLER EXECUTED


    ${customReportData}=    Create Json Download Report    warehouse_status="DOWNLOADFAILED"    product_status="SYNCFAILED"

    ${reportFileName}=   Set Variable   ${tmpdir}/update_report_2018_08_21_09_43_21.json

    Create File  ${reportFileName}  ${customReportData}

    Log File   ${reportFileName}
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UPGRADED
    Should Not Contain    ${output}    UPTODATE
    Should Contain    ${log_contents}    INSTALLER EXECUTED


Test Product Sul Verify Error Will Force Reinstall On Next Update
    # Sul Verified Error will report Install Failed.

    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UPGRADED
    Should Contain    ${log_contents}    INSTALLER EXECUTED


    ${customReportData}=    Create Json Download Report    warehouse_status="INSTALLFAILED"    product_status="SYNCFAILED"

    ${reportFileName}=   Set Variable   ${tmpdir}/update_report_2018_08_21_09_43_21.json

    Create File  ${reportFileName}  ${customReportData}

    Log File   ${reportFileName}
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json   ${tmpdir}/update_report.json

    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UPGRADED
    Should Not Contain    ${output}    UPTODATE
    Should Contain    ${log_contents}    INSTALLER EXECUTED

Test Product Sul Sync Error Will Force Reinstall On Next Update
    # Testing Sul Failing to sync Warehouse Metadata.

    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json
    ${log_contents} =  Dump suldownloader log
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UPGRADED
    Should Contain    ${log_contents}    INSTALLER EXECUTED


    ${customReportData}=    Create Json Download Report    warehouse_status="DOWNLOADFAILED"    product_count=0

    ${reportFileName}=   Set Variable   ${tmpdir}/update_report_2018_08_21_09_43_21.json

    Create File  ${reportFileName}  ${customReportData}

    Log File   ${reportFileName}
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json   ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UPGRADED
    Should Not Contain    ${output}    UPTODATE
    Should Contain    ${log_contents}    INSTALLER EXECUTED

Test Product Sul Download Reports Up To Date when Multiple Report Files Exist

    ${result} =  Perform Install   0  INSTALLER EXECUTED  ${tmpdir}/update_report.json
    ${log_contents} =  Dump suldownloader log
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UPGRADED
    Should Contain    ${log_contents}    INSTALLER EXECUTED


    ${customReportData}=    Create Json Download Report    warehouse_status="DOWNLOADFAILED"    product_count=0

    ${reportFileName}=   Set Variable   ${tmpdir}/update_report_2018_08_21_09_43_21.json

    Create File  ${reportFileName}  ${customReportData}

    Move File   ${tmpdir}/update_report.json   ${tmpdir}/update_report_2018_08_21_09_53_21.json
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Not Contain    ${output}    UPGRADED
    Should Contain    ${output}    UPTODATE
    Should Not Contain    ${log_contents}    INSTALLER EXECUTED


Test SulDownloader Can Download Multiple Products From Multiple Warehouses

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/
    Add Component Warehouse Config   ${EXAMPLE_PLUGIN_RIGID_NAME}  ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/

    #generate install file in directory    ${tmpdir}/TestInstallFiles/
    Create Install File   0   BASE INSTALLER EXECUTED  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}
    Create Install File   0   PLUGIN INSTALLER EXECUTED  ${tmpdir}/TestInstallFiles/${EXAMPLE_PLUGIN_RIGID_NAME}

	Generate Warehouse

    Log File    /opt/sophos-spl/tmp/SDT/temp_warehouse/customer_files/9/53/9539d7d1f36a71bbac1259db9e868231.dat

    Start Update Server    1233    ${tmpdir}/temp_warehouse/customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/

    Can Curl Url    https://localhost:1233
    Can Curl Url    https://localhost:1234/catalogue/sdds.${BASE_RIGID_NAME}.xml
    Can Curl Url    https://localhost:1234/catalogue/sdds.${EXAMPLE_PLUGIN_RIGID_NAME}.xml

    Recreate Installation In Temp Dir
    ${config} =    Create JSON Config    install_path=${tmpdir}/sspl  include_plugins=${TRUE}
    Create File    ${tmpdir}/update_config.json    content=${config}

    ${UninstallMessage} =   Set Variable   UNINSTALLER EXECUTED
    Create Uninstall File   0    Base-${UninstallMessage}   ${BASE_RIGID_NAME}
    Create Uninstall File   0    Plugin-${UninstallMessage}   ${EXAMPLE_PLUGIN_RIGID_NAME}

    Run Process    chmod +x ${InstallProductsDir}/BASE_RIGID_NAME.sh     shell=True
    Run Process    chmod +x ${InstallProductsDir}/EXAMPLE_PLUGIN_RIGID_NAME.sh     shell=True
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Not Contain    ${log_contents}    ${UninstallMessage}
    Should Contain    ${log_contents}    BASE INSTALLER EXECUTED
    Should Contain    ${log_contents}    PLUGIN INSTALLER EXECUTED

Test SulDownloader Can Download Multiple Products From A Single Warehouse

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}   ${BASE_RIGID_NAME}
    Add Component Warehouse Config   ${EXAMPLE_PLUGIN_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${EXAMPLE_PLUGIN_RIGID_NAME}   ${BASE_RIGID_NAME}

    #generate install file in directory    ${tmpdir}/TestInstallFiles/
    Create Install File   0   BASE INSTALLER EXECUTED  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}
    Create Install File   0   PLUGIN INSTALLER EXECUTED  ${tmpdir}/TestInstallFiles/${EXAMPLE_PLUGIN_RIGID_NAME}

	Generate Warehouse

    Log File    /opt/sophos-spl/tmp/SDT/temp_warehouse/customer_files/9/53/9539d7d1f36a71bbac1259db9e868231.dat

    Start Update Server    1233    ${tmpdir}/temp_warehouse/customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/

    Can Curl Url    https://localhost:1233
    Can Curl Url    https://localhost:1234/catalogue/sdds.${BASE_RIGID_NAME}.xml

    Recreate Installation In Temp Dir
    ${config} =    Create JSON Config    install_path=${tmpdir}/sspl    include_plugins=${TRUE}
    Create File    ${tmpdir}/update_config.json    content=${config}

    ${UninstallMessage} =   Set Variable   UNINSTALLER EXECUTED
    Create Uninstall File   0    Base-${UninstallMessage}   ${BASE_RIGID_NAME}
    Create Uninstall File   0    Plugin-${UninstallMessage}   ${EXAMPLE_PLUGIN_RIGID_NAME}

    Run Process    chmod +x ${InstallProductsDir}/BASE_RIGID_NAME.sh     shell=True
    Run Process    chmod +x ${InstallProductsDir}/EXAMPLE_PLUGIN_RIGID_NAME.sh     shell=True

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log
    Log File  /opt/sophos-spl/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Not Contain    ${log_contents}    ${UninstallMessage}
    Should Contain    ${log_contents}    BASE INSTALLER EXECUTED
    Should Contain    ${log_contents}    PLUGIN INSTALLER EXECUTED

Test SulDownloader Can Download Products From An Update Cache
    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}   ${BASE_RIGID_NAME}
    Add Component Warehouse Config   ${EXAMPLE_PLUGIN_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/    ${EXAMPLE_PLUGIN_RIGID_NAME}  ${BASE_RIGID_NAME}

    #generate install file in directory    ${tmpdir}/TestInstallFiles/
    Create Install File   0   BASE INSTALLER EXECUTED  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}
    Create Install File   0   PLUGIN INSTALLER EXECUTED  ${tmpdir}/TestInstallFiles/${EXAMPLE_PLUGIN_RIGID_NAME}

	Generate Warehouse  ${FALSE}
    Log File    /opt/sophos-spl/tmp/SDT/temp_warehouse/customer_files/9/53/9539d7d1f36a71bbac1259db9e868231.dat

    Start Update Server    1236    ${tmpdir}/temp_warehouse/warehouse_root/

    Can Curl Url    https://localhost:1236/sophos/customer
    Can Curl Url    https://localhost:1236/sophos/warehouse/catalogue/sdds.${BASE_RIGID_NAME}.xml

    Recreate Installation In Temp Dir
    ${config} =    Create JSON Update Cache Config    install_path=${tmpdir}/sspl  include_plugins=${TRUE}
    Log   ${config}
    Create File    ${tmpdir}/update_config.json    content=${config}

    ${UninstallMessage} =   Set Variable   UNINSTALLER EXECUTED
    Create Uninstall File   0    Base-${UninstallMessage}   ${BASE_RIGID_NAME}
    Create Uninstall File   0    Plugin-${UninstallMessage}   ${EXAMPLE_PLUGIN_RIGID_NAME}

    Run Process    chmod +x ${InstallProductsDir}/BASE_RIGID_NAME.sh     shell=True
    Run Process    chmod +x ${InstallProductsDir}/EXAMPLE_PLUGIN_RIGID_NAME.sh     shell=True

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log
    Log File  /opt/sophos-spl/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Not Contain    ${log_contents}    ${UninstallMessage}
    Should Contain    ${log_contents}    PLUGIN INSTALLER EXECUTED
    Should Contain    ${log_contents}    BASE INSTALLER EXECUTED


Test SulDownloader Can Fail Over To Update From Sophos When Update Cache Is Offline

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}  ${BASE_RIGID_NAME}
    Add Component Warehouse Config   ${EXAMPLE_PLUGIN_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${EXAMPLE_PLUGIN_RIGID_NAME}   ${BASE_RIGID_NAME}

    #generate install file in directory    ${tmpdir}/TestInstallFiles/
    Create Install File   0   BASE INSTALLER EXECUTED  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}
    Create Install File   0   PLUGIN INSTALLER EXECUTED  ${tmpdir}/TestInstallFiles/${EXAMPLE_PLUGIN_RIGID_NAME}

	Generate Warehouse   ${TRUE}
    Log File    ${SOPHOS_INSTALL}/tmp/SDT/temp_warehouse/customer_files/9/53/9539d7d1f36a71bbac1259db9e868231.dat

    Start Update Server    1233    ${tmpdir}/temp_warehouse/customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/

    Can Curl Url    https://localhost:1233
    Can Curl Url    https://localhost:1234/catalogue/sdds.${BASE_RIGID_NAME}.xml

    Recreate Installation In Temp Dir
    ${config} =    Create JSON Update Cache Config    install_path=${tmpdir}/sspl  include_plugins=${TRUE}
    Log   ${config}
    Create File    ${tmpdir}/update_config.json    content=${config}

    ${UninstallMessage} =   Set Variable   UNINSTALLER EXECUTED
    Create Uninstall File   0    Base-${UninstallMessage}   ${BASE_RIGID_NAME}
    Create Uninstall File   0    Plugin-${UninstallMessage}   ${EXAMPLE_PLUGIN_RIGID_NAME}

    Run Process    chmod +x ${InstallProductsDir}/BASE_RIGID_NAME.sh     shell=True
    Run Process    chmod +x ${InstallProductsDir}/EXAMPLE_PLUGIN_RIGID_NAME.sh     shell=True

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log
    Log File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${SUCCESS}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    SUCCESS
    Should Not Contain    ${log_contents}    ${UninstallMessage}
    Should Contain    ${log_contents}    BASE INSTALLER EXECUTED
    Should Contain    ${log_contents}    PLUGIN INSTALLER EXECUTED


Test SulDownloader Fails To Download From Update Cache If Using Wrong Certificates

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}  ${BASE_RIGID_NAME}
    Add Component Warehouse Config   ${EXAMPLE_PLUGIN_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${EXAMPLE_PLUGIN_RIGID_NAME}  ${BASE_RIGID_NAME}

    #generate install file in directory    ${tmpdir}/TestInstallFiles/
    Create Install File   0   BASE INSTALLER EXECUTED  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}
    Create Install File   0   PLUGIN INSTALLER EXECUTED  ${tmpdir}/TestInstallFiles/${EXAMPLE_PLUGIN_RIGID_NAME}

    Set Environment Variable  CORRUPTINSTALL  yes

	Generate Warehouse  ${FALSE}
    Log File    ${SOPHOS_INSTALL}/tmp/SDT/temp_warehouse/customer_files/9/53/9539d7d1f36a71bbac1259db9e868231.dat

    Start Update Server    1236    ${tmpdir}/temp_warehouse/warehouse_root/

    Can Curl Url    https://localhost:1236/sophos/customer
    Can Curl Url    https://localhost:1236/sophos/warehouse/catalogue/sdds.${BASE_RIGID_NAME}.xml

    Recreate Installation In Temp Dir
    ${config} =    Create JSON Update Cache Config    install_path=${tmpdir}/sspl
    Log   ${config}
    Create File    ${tmpdir}/update_config.json    content=${config}

    ${UninstallMessage} =   Set Variable   UNINSTALLER EXECUTED
    Create Uninstall File   0    Base-${UninstallMessage}   ${BASE_RIGID_NAME}
    Create Uninstall File   0    Plugin-${UninstallMessage}   ${EXAMPLE_PLUGIN_RIGID_NAME}

    Run Process    chmod +x ${InstallProductsDir}/BASE_RIGID_NAME.sh     shell=True
    Run Process    chmod +x ${InstallProductsDir}/EXAMPLE_PLUGIN_RIGID_NAME.sh     shell=True

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log
    Log File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    Error Codes Match   ${result.rc}    ${INSTALLFAILED}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    INSTALLFAILED
    Should Not Contain    ${log_contents}    ${UninstallMessage}
    Should Not Contain    ${log_contents}    BASE INSTALLER EXECUTED
    Should Not Contain    ${log_contents}    PLUGIN INSTALLER EXECUTED


Test SulDownloader times out installs
    [Tags]    SLOW  SULDOWNLOADER
    [Timeout]    15 minutes
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log

    ${ExitCode} =  Set Variable   0
    ${Message} =  Set Variable   LONG DELAY
    ${ConfigRigidname} =  Set Variable  ${BASE_RIGID_NAME}
    ${WarehouseRigidName} =  Set Variable  ${BASE_RIGID_NAME}
    ${ConfigFileCert} =  Set Variable  ${SUPPORT_FILES}/sophos_certs/
    ${rootPath} =  Set Variable    ${tmpdir}/TestInstallFiles/${ConfigRigidname}

    Create Directory    ${tmpdir}/TestInstallFiles/
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...    echo '${Message}'
    ...    sleep 700
    ...    echo 'AFTER SLEEP'
    ...    exit ${ExitCode}
    ...    \
    Create File    ${rootPath}/install.sh    content=${script}

    Log  ${ConfigRigidname}
    Log  ${WarehouseRigidName}

    Create Warehouse for tmp product  ${ConfigRigidname}  ${WarehouseRigidName}

    Start Warehouse servers  ${WarehouseRigidName}

    Create SulDownloader Config  ${WarehouseRigidName}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json     ${tmpdir}/update_report.json

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json
    Log File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Error Codes Match   ${result.rc}    ${INSTALLFAILED}

    ${output} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${output}    INSTALLFAILED
    Should Contain    ${log_contents}   LONG DELAY
    Should Not Contain    ${result.stdout}   AFTER SLEEP
    Should Not Contain    ${log_contents}   AFTER SLEEP
    Should Not Contain    ${result.stdout}    INSTALLER EXECUTED
    Should Not Contain    ${log_contents}    INSTALLER EXECUTED


Test That Only One SulDownloader Can Run At One Time
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Create Install File   0   Installer Executed  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}  5
    Create Warehouse for tmp product  ${BASE_RIGID_NAME}  ${BASE_RIGID_NAME}
    Start Warehouse servers  ${BASE_RIGID_NAME}
    Create SulDownloader Config  ${BASE_RIGID_NAME}

    ${Sul_Handle} =  Start Process  ${SUL_DOWNLOADER}  ${tmpdir}/update_config.json  ${tmpdir}/update_report.json
    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  File Should Exist  ${SOPHOS_INSTALL}/var/lock/suldownloader.pid
    ${result} =  Run Process  ${SUL_DOWNLOADER}  ${tmpdir}/update_config.json  ${tmpdir}/output2.json
    Should Be Equal As Integers  ${result.rc}  ${SULDOWNLOADERLOCKFILEERROR}  Second Instance Of SulDownloader Didn't Fail As Expected
    Wait For Process  ${Sul_Handle}
    File Should Not Exist  ${SOPHOS_INSTALL}/var/lock/suldownloader.pid

    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log
    Should Contain  ${log_contents}  Installer Executed
    Should Contain  ${log_contents}  Failed to lock file

Test That Suldownloader Checks Update Caches and Proxies in the Correct Order
    [Timeout]  10 minutes  #This test takes around 7 minutes on amazon linux we think due to long connection timeouts
    # Create config.json file with an update cache and a authenticated policy proxy
    ${config} =    Create Update Cache Config   install_path=${tmpdir}/sspl
    ${proxy_url} =    Set Variable    http://localhost:1235/
    Set to Dictionary    ${config["proxy"]}    url=${proxy_url}
    Set to Dictionary    ${config["proxy"]["credential"]}  username=user  password=pass  proxyType=1
    Log Dictionary    ${config}
    ${config_json} =    Data to JSON    ${config}
    Create File  ${tmpdir}/update_config.json  content=${config_json}
    # Add a saved environment proxy file to mock one that is potentially created on install
    ${saved_env_proxy} =  Set Variable  https://saved-env-proxy.com
    Create File  ${tmpdir}/sspl/base/etc/savedproxy.config  content=${saved_env_proxy}
    # Start suldownloader with an environment proxy
    ${Sul_Handle} =  Run Process  ${SUL_DOWNLOADER}  ${tmpdir}/update_config.json  ${tmpdir}/update_report.json  env:https_proxy=10.144.1.10:8080
    Log    "stdout = ${Sul_Handle.stdout}"
    Log    "stderr = ${Sul_Handle.stderr}"
    ${update_cache_message1} =  Set Variable  Update cache at localhost:1235
    ${update_cache_message2} =  Set Variable  Update cache at localhost:1236
    ${policy_proxy_message} =  Set Variable  Proxy used was: "${proxy_url}"
    ${env_proxy_message} =  Set Variable  Proxy used was: "environment:"
    ${saved_env_proxy_message} =  Set Variable  Proxy used was: "${saved_env_proxy}"
    ${no_proxy_message} =  Set Variable  Proxy used was: "noproxy:"
    Check Suldownloader Log Contains In Order
        ...  ${update_cache_message1}
        ...  ${update_cache_message2}
        ...  ${policy_proxy_message}
        ...  ${env_proxy_message}
        ...  ${saved_env_proxy_message}
        ...  ${no_proxy_message}

Test Suldownloader Can Install From Warehouse Without Base Version
    Create Install File   0   INSTALLER EXECUTED    ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}

	Generate Warehouse      RemoveBaseVersion=True
    Require Update Server

    #Base version is not set on config by default
    Setup SulDownloader Config File

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Verify Product Installed and Report Upgraded   ${result}


Test Suldownloader Can Install From Warehouse With Base Version 1 And Config Set to Version 1
    Create Install File   0   INSTALLER EXECUTED    ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}

	Generate Warehouse      DIST_MAJOR=1
    Require Update Server

    &{List} =  Create Dictionary   rigidName=${BASE_RIGID_NAME}  tag=RECOMMENDED  baseVersion=1

    Setup SulDownloader Config File    primarySubscription=&{List}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Verify Product Installed and Report Upgraded   ${result}


Test Suldownloader Can Install From Warehouse With Base Version 1 And Config Without BaseVersion
    Create Install File   0   INSTALLER EXECUTED    ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}

	Generate Warehouse      DIST_MAJOR=1
    Require Update Server

    #Base version is not set on config by default
    Setup SulDownloader Config File

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Verify Product Installed and Report Upgraded   ${result}


Test Suldownloader Can Upgrade From Major Versions Using Warehouses
    Create Install File   0   INSTALLER EXECUTED    ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}

	Generate Warehouse      DIST_MAJOR=1
    Require Update Server

    Setup SulDownloader Config File    remove_entries=baseVersion

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Verify Product Installed and Report Upgraded   ${result}

    Stop Update Server

    Remove Directory  ${tmpdir}/TestInstallFiles/   recursive=True
    Remove Directory  ${tmpdir}/temp_warehouse/    recursive=True

    Clear Warehouse Config

    Create Install File   0   INSTALLER EXECUTED2    ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}

	Generate Warehouse      DIST_MAJOR=2
    Require Update Server

    # do not recreate the config file. Use the same

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Verify Product Installed and Report Upgraded   ${result}


Test Suldownloader Can Download A Component Suite And Component From A Single Warehouse
    Create Product File   helloworld   ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}
    Create Install File   0   INSTALLER EXECUTED    ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}2
    Create Install File   0   INSTALLER EXECUTED    ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}3
    Create Install File   0   INSTALLER EXECUTED    ${tmpdir}/TestInstallFiles/${EXAMPLE_PLUGIN_RIGID_NAME}

    ##Add Component Suite Warehouse Config:  rigidname, source directory, target directory, warehouse name
    ##Add Component Warehouse Config:  rigidname, source directory, target directory, component parent, warehouse name
    Add Component Suite Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   Warehouse1
    Add Component Warehouse Config   ${BASE_RIGID_NAME}2   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}  Warehouse1
    Add Component Warehouse Config   ${BASE_RIGID_NAME}3   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}  Warehouse1
    Add Component Warehouse Config   ${EXAMPLE_PLUGIN_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${EXAMPLE_PLUGIN_RIGID_NAME}  Warehouse1

	Generate Warehouse

	Log File  ${tmpdir}/temp_warehouse/${BASE_RIGID_NAME}/sdds/SDDS-Import.xml
	Log File  ${tmpdir}/temp_warehouse/${BASE_RIGID_NAME}2/sdds/SDDS-Import.xml
	Log File  ${tmpdir}/temp_warehouse/${BASE_RIGID_NAME}3/sdds/SDDS-Import.xml
	Log File  ${tmpdir}/temp_warehouse/${EXAMPLE_PLUGIN_RIGID_NAME}/sdds/SDDS-Import.xml

    Require Update Server

    Setup SulDownloader Config File    remove_entries=baseVersion   include_plugins=${True}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Verify Product Installed and Report Upgraded   ${result}

    # Check all expected files exist in generated CID location
    
    # is defined in the component suite, but will not be downloaded, as suldownloader will target the components inside the component suite
    File Should Not Exist  ${tmpdir}/sspl/base/update/cache/primary/${BASE_RIGID_NAME}/testinstall.txt  
    
    File Should Exist  ${tmpdir}/sspl/base/update/cache/primary/${BASE_RIGID_NAME}2/install.sh   # comes from ${BASE_RIGID_NAME}2
    File Should Exist  ${tmpdir}/sspl/base/update/cache/primary/${BASE_RIGID_NAME}3/install.sh   # comes from ${BASE_RIGID_NAME}3
    File Should Exist  ${tmpdir}/sspl/base/update/cache/primary/${EXAMPLE_PLUGIN_RIGID_NAME}/install.sh

    Stop Update Server

Test Suldownloader Can Download A Component Suite And Component From A Multiple Warehouses
    Create Product File   helloworld   ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}
    Create Install File   0   INSTALLER EXECUTED    ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}2
    Create Install File   0   INSTALLER EXECUTED    ${tmpdir}/TestInstallFiles/${EXAMPLE_PLUGIN_RIGID_NAME}

    ##Add Component Suite Warehouse Config:  rigidname, source directory, target directory, warehouse name
    ##Add Component Warehouse Config:  rigidname, source directory, target directory, component parent, warehouse name
    Add Component Suite Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   Warehouse1
    Add Component Warehouse Config   ${BASE_RIGID_NAME}2   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}  Warehouse1
    Add Component Warehouse Config   ${EXAMPLE_PLUGIN_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${EXAMPLE_PLUGIN_RIGID_NAME}  Warehouse2

	Generate Warehouse

	Log File  ${tmpdir}/temp_warehouse/${BASE_RIGID_NAME}/sdds/SDDS-Import.xml
	Log File  ${tmpdir}/temp_warehouse/${BASE_RIGID_NAME}2/sdds/SDDS-Import.xml
	Log File  ${tmpdir}/temp_warehouse/${EXAMPLE_PLUGIN_RIGID_NAME}/sdds/SDDS-Import.xml

    Require Update Server

    Setup SulDownloader Config File   remove_entries=baseVersion   include_plugins=${True}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Verify Product Installed and Report Upgraded   ${result}

    # Check all expected files exist in generated CID location

    # described in the component suite, but not installed as suldownloader will expand to the components inside the warehouse
    File Should Not Exist  ${tmpdir}/sspl/base/update/cache/primary/${BASE_RIGID_NAME}/testinstall.txt  

    File Should Exist  ${tmpdir}/sspl/base/update/cache/primary/${BASE_RIGID_NAME}2/install.sh   # comes from ${BASE_RIGID_NAME}2
    File Should Exist  ${tmpdir}/sspl/base/update/cache/primary/${EXAMPLE_PLUGIN_RIGID_NAME}/install.sh

    Stop Update Server


Suldownloader Should Keep Reporting Failure While Verification Fails
    Create Install File   0   INSTALLER EXECUTED    ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}

	Generate Warehouse      RemoveBaseVersion=True

    Require Update Server

    Setup SulDownloader Config File

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Verify Product Installed and Report Upgraded   ${result}

    Move File   ${tmpdir}/update_report.json  ${tmpdir}/update_report_first.json

    Stop Update Server
    Remove Directory       ${tmpdir}/temp_warehouse   Recursive=True

    Clear Warehouse Config
    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}

	Generate Warehouse      RemoveBaseVersion=True   CORRUPTINSTALL=yes
    Require Update Server

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check Suldownloader Log Contains In Order  Checking signature.
    Verify SulDownloader Failed With The Expected Errors  ${result}   ${INSTALLFAILED}   INSTALLFAILED  failed signature verification
    Move File   ${tmpdir}/update_report.json  ${tmpdir}/update_report_second.json

    # run second time and expect it to continue to fail
    Log File      ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Remove File   ${SOPHOS_INSTALL}/logs/base/suldownloader.log

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json

    Check Suldownloader Log Contains In Order  Checking signature.
    Verify SulDownloader Failed With The Expected Errors  ${result}   ${INSTALLFAILED}   INSTALLFAILED  failed signature verification

Test SulDownloader Will Obtain Dictionary Values For Product Names In Warehouse

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/
    Add Component Warehouse Config   ${EXAMPLE_PLUGIN_RIGID_NAME}  ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/

    #generate install file in directory    ${tmpdir}/TestInstallFiles/
    Create Install File   0   BASE INSTALLER EXECUTED  ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}
    Create Install File   0   PLUGIN INSTALLER EXECUTED  ${tmpdir}/TestInstallFiles/${EXAMPLE_PLUGIN_RIGID_NAME}

	Generate Warehouse

    Log File    /opt/sophos-spl/tmp/SDT/temp_warehouse/customer_files/9/53/9539d7d1f36a71bbac1259db9e868231.dat

    Start Update Server    1233    ${tmpdir}/temp_warehouse/customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/

    Can Curl Url    https://localhost:1233
    Can Curl Url    https://localhost:1234/catalogue/sdds.${BASE_RIGID_NAME}.xml
    Can Curl Url    https://localhost:1234/catalogue/sdds.${EXAMPLE_PLUGIN_RIGID_NAME}.xml

    Recreate Installation In Temp Dir
    ${config} =    Create JSON Config    install_path=${tmpdir}/sspl  include_plugins=${TRUE}
    Create File    ${tmpdir}/update_config.json    content=${config}

    ${UninstallMessage} =   Set Variable   UNINSTALLER EXECUTED
    Create Uninstall File   0    Base-${UninstallMessage}   ${BASE_RIGID_NAME}
    Create Uninstall File   0    Plugin-${UninstallMessage}   ${EXAMPLE_PLUGIN_RIGID_NAME}

    Run Process    chmod +x ${InstallProductsDir}/BASE_RIGID_NAME.sh     shell=True
    Run Process    chmod +x ${InstallProductsDir}/EXAMPLE_PLUGIN_RIGID_NAME.sh     shell=True
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${tmpdir}/update_report.json
    ${log_contents} =   Get File   ${tmpdir}/sspl/logs/base/suldownloader.log

    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Log File  ${tmpdir}/update_report.json

    ${reportOutput} =    Get File    ${tmpdir}/update_report.json
    Should Contain    ${reportOutput}    "productName": "Sophos Server Protection for Linux ServerProtectionLinux-Base v0.5.0",
    Should Contain    ${reportOutput}    "productName": "Sophos Server Protection for Linux ServerProtectionLinux-Plugin-Example v0.5.0",


*** Keywords ***

Check SulDownloader Result
    [Arguments]  ${result}  ${expectedExitCode}  @{expectedStdErrLines}
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Error Codes Match   ${result.rc}    ${expectedExitCode}
    Check String Contains Entries  ${result.stderr}  @{expectedStdErrLines}

Check SulDownloader Log Contains
    [Arguments]  @{expectedLines}
    ${logfile}=  Get File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Check String Contains Entries  ${logfile}  @{expectedLines}

Check SulDownloader Log Should Not Contain
    [Arguments]  @{expectedLines}
    ${logfile}=  Get File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Check String Does Not Contain  ${logfile}  @{expectedLines}


Check SulDownloader Report Contains
    [Arguments]   @{expectedLines}
    ${report} =    Get File    ${tmpdir}/update_report.json
    Check String Contains Entries  ${report}  @{expectedLines}


Verify SulDownloader Connects Via Environment Proxy
    Check SulDownloader Log Contains
    ...    Try connection: Sophos at https://localhost:1233 \ via environment proxy

    Check SulDownloader Log Contains
    ...    Successfully connected to: Sophos at https://localhost:1233 \ via environment proxy

    Check SulDownloader Log Should Not Contain
    ...    Failed to connect to Sophos at https://localhost:1233 \ via environment proxy


Verify SulDownloader Connects Via Proxy
    [Arguments]  ${proxy_url}

    Check SulDownloader Log Contains
    ...    Try connection: Sophos at https://localhost:1233 via proxy: ${proxy_url}

    Check SulDownloader Log Contains
    ...    Successfully connected to: Sophos at https://localhost:1233 via proxy: ${proxy_url}

    Check SulDownloader Log Should Not Contain
    ...    Failed to connect to Sophos at https://localhost:1233 via proxy: ${proxy_url}



Create Install File
    [Arguments]   ${Exitcode}   ${Message}  ${rootPath}=${tempdir}/TestInstallFiles/${BASE_RIGID_NAME}  ${Sleep}=0  ${installsh}=install.sh
    Create Directory    ${tmpdir}/TestInstallFiles/
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...    echo '${Message}'
    ...    sleep ${Sleep}
    ...    exit ${ExitCode}
    ...    \
    Create File    ${rootPath}/${installsh}    content=${script}

Create Product File
    [Arguments]   ${fileContent}   ${rootPath}=${tempdir}/TestInstallFiles/${BASE_RIGID_NAME}  ${Sleep}=0
    Create Directory    ${tmpdir}/TestInstallFiles/
    Create File    ${rootPath}/testinstall.txt    content=${fileContent}


Create Success Or Fail Install File Depending On Existence Of File
    [Arguments]   ${SuccessMessage}  ${FailMessage}  ${FilePath}  ${RigidName}=${BASE_RIGID_NAME}
    Create Directory    ${tmpdir}/TestInstallFiles/${RigidName}
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...    if [ -e ${FilePath} ]
    ...       then
    ...       echo '${SuccessMessage}'
    ...       exit 0
    ...    else
    ...       echo '${FailMessage}'
    ...       exit 1
    ...    fi
    ...    \
    Create File    ${tmpdir}/TestInstallFiles/${RigidName}/install.sh    content=${script}


Create Uninstall File
    [Arguments]    ${Exitcode}   ${Message}   ${ProductName}
    ${script} =     Catenate    SEPARATOR=\n
    ...    \#!/bin/bash
    ...    echo '${Message}'
    ...    exit ${ExitCode}
    ...    \
    Create File   ${tmpdir}/${ProductName}.sh    content=${script}
    ${result} =   Run Process   ln  -s  ${tmpdir}/${ProductName}.sh  ${InstallProductsDir}/${ProductName}.sh
    Should Be Equal As Integers    ${result.rc}    0        Failed to create symlink: ln -s ${tmpdir}/${ProductName}.sh ${InstallProductsDir}/${ProductName}.sh


Convert Error Code Into String
    [Arguments]   ${ErrorCode}
    Run Keyword If   ${ErrorCode}==${SUCCESS}                      Return From Keyword   SUCCESS
    Run Keyword If   ${ErrorCode}==${INSTALLFAILED}                Return From Keyword   INSTALLFAILED
    Run Keyword If   ${ErrorCode}==${DOWNLOADFAILED}               Return From Keyword   DOWNLOADFAILED
    Run Keyword If   ${ErrorCode}==${RESTARTNEEDED}                Return From Keyword   RESTARTNEEDED
    Run Keyword If   ${ErrorCode}==${CONNECTIONERROR}              Return From Keyword   CONNECTIONERROR
    Run Keyword If   ${ErrorCode}==${PACKAGESOURCEMISSING}         Return From Keyword   PACKAGESOURCEMISSING
    Run Keyword If   ${ErrorCode}==${UNINSTALLFAILED}              Return From Keyword   UNINSTALLFAILED
    Run Keyword If   ${ErrorCode}==${UNSPECIFIED}                  Return From Keyword   UNSPECIFIED
    Run Keyword If   ${ErrorCode}==${INVALIDCOMMANDLINEARGUMENTS}  Return From Keyword   INVALIDCOMMANDLINEARGUMENTS
    Run Keyword If   ${ErrorCode}==${FILEREADORWRITEERROR}         Return From Keyword   FILEREADORWRITEERROR
    Return From Keyword  UKNOWNERRORCODE

Error Codes Match
    [Arguments]   ${ACTUAL}   ${EXPECTED}
    Log  ${ACTUAL}
    Log  ${EXPECTED}
    ${ActualString} =    Convert Error Code Into String   ${ACTUAL}
    ${ExpectedString} =    Convert Error Code Into String   ${EXPECTED}
    Should Be Equal As Strings    ${ActualString}    ${ExpectedString}

Check Proxy Log Contains
    [Arguments]  ${pattern}  ${fail_message}="proxy does not contain message"
    ${ret} =  Grep File  ./tmp/proxy_server.log  ${pattern}
    Should Contain  ${ret}  ${pattern}  ${fail_message}


Create Warehouse for tmp product
    [Arguments]  ${ConfigRigidname}=${BASE_RIGID_NAME}  ${WarehouseRigidName}=${BASE_RIGID_NAME}
    Clear Warehouse Config

    Add Component Warehouse Config   ${ConfigRigidname}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${ConfigRigidname}   ${WarehouseRigidName}

	Generate Warehouse

Start Warehouse servers
    [Arguments]  ${WarehouseRigidName}=${BASE_RIGID_NAME}
    Start Update Server    1233    ${tmpdir}/temp_warehouse/customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1233
    Can Curl Url    https://localhost:1234/catalogue/${WarehouseRigidName}

Create SulDownloader Config
    [Arguments]  ${WarehouseRigidName}=${BASE_RIGID_NAME}
    ...          ${ConfigFileCert}=${SUPPORT_FILES}/sophos_certs/
    ${config} =    Create JSON Config    install_path=${tmpdir}/sspl   rigidname=${WarehouseRigidName}
    Copy File   ${SUPPORT_FILES}/sophos_certs/ps_rootca.crt  ${tmpdir}/sspl/base/update/rootcerts/
    Copy File   ${SUPPORT_FILES}/sophos_certs/rootca.crt  ${tmpdir}/sspl/base/update/rootcerts/
    Create File    ${tmpdir}/update_config.json    content=${config}
    Log File   ${tmpdir}/update_config.json

# Note that the default values set here are also reproduced in def create_config the SulDownloader.py library file
Perform Install
    [Arguments]  ${ExitCode}  ${Message}   ${OutputJsonFile}=${tmpdir}/update_report.json
    ...          ${ConfigRigidname}=${BASE_RIGID_NAME}   ${WarehouseRigidName}=${BASE_RIGID_NAME}
    ...          ${ConfigFileCert}=${SUPPORT_FILES}/sophos_certs/
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Create Install File   ${ExitCode}   ${Message}  ${tmpdir}/TestInstallFiles/${ConfigRigidname}
    create directory   ${tmpdir}/sspl/var/
    create directory   ${tmpdir}/sspl/var/lock/
    create file   ${tmpdir}/sspl/base/etc/logger.conf   VERBOSITY = DEBUG
    Log  ${ConfigRigidname}
    Log  ${WarehouseRigidName}

    Create Warehouse for tmp product  ${ConfigRigidname}  ${WarehouseRigidName}

    Start Warehouse servers  ${WarehouseRigidName}

    Create SulDownloader Config  ${WarehouseRigidName}

    ${result} =    Run Process    ${SUL_DOWNLOADER}    ${tmpdir}/update_config.json    ${OutputJsonFile}  env:SOPHOS_INSTALL=${tmpdir}/sspl
    [Return]  ${result}

Require Update Server
    Start Update Server    1233    ${tmpdir}/temp_warehouse/customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1233
    Can Curl Url    https://localhost:1234/catalogue/sdds.${BASE_RIGID_NAME}.xml



Require Warehouse With Fake Single Installer Product
    #generate install file in directory    ${tmpdir}/TestInstallFiles/
    Create Install File   0   INSTALLER EXECUTED    ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}
    Create File   ${tmpdir}/TestInstallFiles/${BASE_RIGID_NAME}/VERSION.ini   PRODUCT_NAME = Sophos Server Protection Linux - Base Component\nPRODUCT_VERSION = 9.9.9.999\nBUILD_DATE = 2020-11-09

    Add Component Warehouse Config   ${BASE_RIGID_NAME}   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ${BASE_RIGID_NAME}

	Generate Warehouse
    Require Update Server

Verify SulDownloader Failed With The Expected Errors
    [Arguments]  ${result}   ${failedCode}  ${reportError1}  ${reportError2}
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Error Codes Match   ${result.rc}    ${failedCode}
    ${output} =    Get File    ${tmpdir}/update_report.json
    Log    "output JSON = ${output}"
    Should Contain    ${output}    ${reportError1}
    Should Contain    ${output}    ${reportError2}


Verify Product Installed and Report Upgraded
    [Arguments]  ${result}
    Log    "stdout = ${result.stdout}"
    Log    "stderr = ${result.stderr}"
    Error Codes Match   ${result.rc}    ${SUCCESS}
    ${output} =    Get File    ${tmpdir}/update_report.json
    Log    "output JSON = ${output}"
    Should Contain    ${output}    SUCCESS
    Should Contain    ${output}    UPGRADED

Setup SulDownloader Config File
   [Arguments]  &{kwargs}
   ${config} =    Create Config    &{kwargs}
   ${config_json} =    Data to JSON    ${config}
   Log    "config_json = ${config_json}"
   Create File    ${tmpdir}/update_config.json    content=${config_json}

