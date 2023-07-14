*** Settings ***
Library    OperatingSystem
Library    Process

Library    ${LIB_FILES}/CentralUtils.py
Library    ${LIB_FILES}/FullInstallerUtils.py
Library    ${LIB_FILES}/MCSRouter.py
Library    ${LIB_FILES}/OSUtils.py
Library    ${LIB_FILES}/OnFail.py
Library    ${LIB_FILES}/TeardownTools.py
Library    ${LIB_FILES}/WarehouseUtils.py

Resource    GeneralTeardownResources.robot


*** Keywords ***
Upgrade Resources Suite Setup
    Set Suite Variable    ${GL_handle}       ${EMPTY}
    Set Suite Variable    ${GL_UC_handle}    ${EMPTY}
    generate_local_ssl_certs_if_they_dont_exist
    Install Local SSL Server Cert To System
    Regenerate Certificates
    Set_Local_CA_Environment_Variable

Upgrade Resources Suite Teardown
    Run Process    make    clean    cwd=${SUPPORT_FILES}/https/

Upgrade Resources Test Setup
    require_uninstalled
    Should Not Exist    ${SOPHOS_INSTALL}
    Set Environment Variable    CORRUPTINSTALL    no

Upgrade Resources SDDS3 Test Teardown
    Stop Local SDDS3 Server
    General Test Teardown
    run_cleanup_functions
    Run Keyword If Test Failed    Dump Teardown Log    /tmp/preserve-sul-downgrade
    Remove File    /tmp/preserve-sul-downgrade
    stop_local_cloud_server
    require_uninstalled


Install Local SSL Server Cert To System
    Copy File    ${SUPPORT_FILES}/https/ca/root-ca.crt.pem    ${SUPPORT_FILES}/https/ca/root-ca.crt
    install_system_ca_cert    ${SUPPORT_FILES}/https/ca/root-ca.crt

Regenerate Certificates
    ${result} =  Run Process    make    cleanCerts    cwd=${SUPPORT_FILES}/CloudAutomation  stderr=STDOUT  env:OPENSSL_CONF=../https/openssl.cnf  env:RANDFILE=.rnd
    Should Be Equal As Integers 	${result.rc} 	0
    Log 	cleanCerts output: ${result.stdout}

    Generate Local Fake Cloud Certificates

Generate Local Fake Cloud Certificates
    #Note RANDFILE env variable is required to create the certificates on amazon
    ${result} =  Run Process    make    all           cwd=${SUPPORT_FILES}/CloudAutomation  stderr=STDOUT  env:OPENSSL_CONF=../https/openssl.cnf  env:RANDFILE=.rnd
    Log 	all output: ${result.stdout}

    # You need execute permission on all folders up to the certificate file to use it in mcsrouter (lower priv user)
    # This makes the necessary folder on jenkins and amazon have the correct permissions. Ubuntu has a+x on home folders
    # by default.
    Run Keyword And Ignore Error  Run Process    chmod  a+x  /home/jenkins  #RHEL and CentOS
    Run Keyword And Ignore Error  Run Process    chmod  a+x  /root          #Amazon

    File Should Exist  ${SUPPORT_FILES}/CloudAutomation/server-private.pem
    File Should Exist  ${SUPPORT_FILES}/CloudAutomation/root-ca.crt.pem


Start Local SDDS3 Server
    [Arguments]    ${launchdarklyPath}=${INPUT_DIRECTORY}/launchdarkly    ${sdds3repoPath}=${VUT_WAREHOUSE_REPO_ROOT}
    ${handle}=  Start Process  python3 ${LIB_FILES}/SDDS3server.py --launchdarkly ${launchdarklyPath} --sdds3 ${sdds3repoPath}  shell=true
    Set Suite Variable    ${GL_handle}    ${handle}

Start Local Dogfood SDDS3 Server
    Start Local SDDS3 Server    ${INPUT_DIRECTORY}/dogfood_launch_darkly    ${DOGFOOD_WAREHOUSE_REPO_ROOT}

Start Local Current Shipping SDDS3 Server
    Start Local SDDS3 Server    ${INPUT_DIRECTORY}/current_shipping_launch_darkly    ${CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT}

Stop Local SDDS3 Server
    return from keyword if    "${GL_handle}" == "${EMPTY}"
    Terminate Process    ${GL_handle}    True
    Set Suite Variable    ${GL_handle}    ${EMPTY}
    dump_teardown_log    /tmp/sdds3_server.log
    Terminate All Processes  True


Send ALC Policy And Prepare For Upgrade
    [Arguments]    ${policyPath}
    Prepare Installation For Upgrade Using Policy    ${policyPath}
    send_policy_file    alc    ${policyPath}    wait_for_policy=${True}

Prepare Installation For Upgrade Using Policy
    [Arguments]    ${policyPath}
    install_upgrade_certs_for_policy  ${policyPath}


Mark Known Upgrade Errors
    # TODO LINUXDAR-7318 - expected till bugfix is in released version
    mark_expected_error_in_log    ${BASE_LOGS_DIR}/watchdog.log  /opt/sophos-spl/base/bin/UpdateScheduler died with signal 9

    # LINUXDAR-4015 There won't be a fix for this error, please check the ticket for more info
    mark_expected_error_in_log    ${RTD_DIR}/log/runtimedetections.log  runtimedetections <> Could not enter supervised child process