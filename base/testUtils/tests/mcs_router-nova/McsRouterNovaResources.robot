*** Settings ***
Documentation    Shared keywords for MCS Router tests

Resource  ../mcs_router/McsRouterResources.robot
Resource  ../watchdog/LogControlResources.robot
Resource  ../mdr_plugin/MDRResources.robot
Resource  ../scheduler_update/SchedulerUpdateResources.robot
Resource  ../GeneralTeardownResource.robot

Library     ${libs_directory}/WarehouseGenerator.py
Library     ${libs_directory}/FullInstallerUtils.py
Library     ${libs_directory}/CentralUtils.py
Library     ${libs_directory}/LogUtils.py
Library     ${libs_directory}/UpdateSchedulerHelper.py
Library     ${libs_directory}/MCSRouter.py


*** Variables ***
${tmpdir}               ${SOPHOS_INSTALL}/tmp/SDT
${sulConfigPath}        ${SOPHOS_INSTALL}/base/update/var/update_config.json
${statusPath}           ${SOPHOS_INSTALL}/base/mcs/status/ALC_status.xml

*** Keywords ***
Check for timeout
    Wait Until Keyword Succeeds
    ...  360 secs
    ...  30 secs
    ...  Check Mcsrouter Log Contains       Failed direct connection to mcs.sandbox.sophos:443 timed out

Dump Logs And Clean Up Temp Dir
    Run Keyword If Test Failed  Log File    /etc/hosts
    Stop Update Server
    Remove Directory   ${tmpdir}  recursive=true
    Reload Cloud Options

Wait For Update Action To Be Processed
    Wait Until Keyword Succeeds
    ...  60 secs
    ...  5 secs
    ...  File Should Exist   ${SOPHOS_INSTALL}/logs/base/suldownloader.log

Setup Warehouse In Localhost
    ${dist} =  Get Folder With Installer
    Copy Directory  ${dist}  ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Base

    ${mdr_component_suite} =  Get SSPL MDR Component Suite
    Copy MDR Component Suite To   ${tmpdir}/TestInstallFiles   mdr_component_suite=${mdr_component_suite}

    #Replace the SophosMDR executable with a fake to avoid unwanted connections
    ${fakeMDRScript} =  Create Fake MDR Executable Contents
    Remove File  ${tmpdir}/TestInstallFiles/${mdr_component_suite.dbos.rigid_name}/files/plugins/mtr/dbos/SophosMTR
    Create File  ${tmpdir}/TestInstallFiles/${mdr_component_suite.dbos.rigid_name}/files/plugins/mtr/dbos/SophosMTR  ${fakeMDRScript}

    Clear Warehouse Config

    Add Component Warehouse Config    ServerProtectionLinux-Base   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ServerProtectionLinux-Base  Warehouse1
    Add Component Suite Warehouse Config   ${mdr_component_suite.mdr_suite.rigid_name}  ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   Warehouse1
    Add Component Warehouse Config   ${mdr_component_suite.mdr_plugin.rigid_name}  ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/  ${mdr_component_suite.mdr_suite.rigid_name}  Warehouse1
    Add Component Warehouse Config   ${mdr_component_suite.dbos.rigid_name}  ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/  ${mdr_component_suite.mdr_suite.rigid_name}  Warehouse1
    Add Component Warehouse Config   ${mdr_component_suite.osquery.rigid_name}  ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/  ${mdr_component_suite.mdr_suite.rigid_name}  Warehouse1

Require Warehouse In Localhost
    Setup Warehouse In Localhost
    Generate Warehouse   MDR_FEATURES=MDR  MDR_Control_FEATURES=MDR  MDR_DBOS_Component_FEATURES=MDR  SDDS_SSPL_OSQUERY_COMPONENT_FEATURES=MDR

    Start Update Server    1233    ${tmpdir}/temp_warehouse//customer_files/
    Start Update Server    1234    ${tmpdir}/temp_warehouse/warehouses/sophosmain/
    Can Curl Url    https://localhost:1234/catalogue/sdds.ServerProtectionLinux-Base.xml
    Can Curl Url    https://localhost:1233
    Copy File   SupportFiles/https/ca/root-ca.crt.pem    SupportFiles/https/ca/root-ca.crt
    Install System Ca Cert  SupportFiles/https/ca/root-ca.crt

Require Update Cache Warehouse In Localhost
    Setup Warehouse In Localhost
    Generate Warehouse  ${FALSE}   MDR_FEATURES=MDR
    Log File    /opt/sophos-spl/tmp/SDT/temp_warehouse/customer_files/9/53/9539d7d1f36a71bbac1259db9e868231.dat

    Start Update Server    1236    ${tmpdir}/temp_warehouse/warehouse_root/

    Can Curl Url    https://localhost:1236/sophos/customer
    Can Curl Url    https://localhost:1236/sophos/warehouse/catalogue/sdds.${BASE_RIGID_NAME}.xml
    Copy File   SupportFiles/https/ca/root-ca.crt.pem    SupportFiles/https/ca/root-ca.crt
    Install System Ca Cert  SupportFiles/https/ca/root-ca.crt

Wait For Config File
    [Arguments]     ${expectedContent}  ${filePath}
    Wait Until Keyword Succeeds
    ...  2 min
    ...  2 secs
    ...  Check Specific File Content  ${expectedContent}  ${filePath}
    Log File  ${filePath}

Check Specific File Content
    [Arguments]     ${expectedContent}  ${filePath}
    ${FileContents} =  Get File  ${filePath}
    Should Contain    ${FileContents}   ${expectedContent}


Check Specific File Does Not Contain Content
    [Arguments]     ${expectedContent}  ${filePath}
    ${FileContents} =  Get File  ${filePath}
    Should Not Contain    ${FileContents}   ${expectedContent}

Setup Fresh Install Nova
    Require Fresh Install
    Set Nova MCS CA Environment Variable

    Set Test Variable    ${certPath}    ${SOPHOS_INSTALL}/base/update/certs
    Generate Update Certs
    Remove Files  ${certPath}/ps_rootca.crt  ${certPath}/ps_rootca.crt.0  ${certPath}/rootca.crt   ${certPath}/rootca.crt.0  ${certPath}/cache_certificates.crt
    Copy File   SupportFiles/sophos_certs/ps_rootca.crt    ${SOPHOS_INSTALL}/base/update/certs
    Copy File   SupportFiles/sophos_certs/rootca.crt    ${SOPHOS_INSTALL}/base/update/certs
    Log File   /etc/hosts


Require Registered
    [Arguments]   ${waitForALCPolicy}=${False}
    Log To Console  ${regCommand}
    Register With Central  ${regCommand}
    Check MCS Router Running
    Wait For Server In Cloud
    Wait Until Keyword Succeeds
    ...  30
    ...  5
    ...  Check Update Scheduler Running

    Run Keyword If   ${waitForALCPolicy}==${True}
    ...  Check ALC Policy Exists

Wait until MCS policy is Downloaded
    Wait Until Keyword Succeeds
    ...  60
    ...  5 secs
    ...  Check Policy Files Exists  MCS-25_policy.xml

Wait until MDR policy is Downloaded
    Wait Until Keyword Succeeds
    ...  120
    ...  20 secs
    ...  Check Policy Files Exists  MDR_policy.xml

Require SDDS urls redirected to Localhost
    [Arguments]   ${usingUpdateCache}=${False}   ${checkAlcPolicy}=${True}

    Run Keyword If   ${checkAlcPolicy}==${True}
    ...   Check ALC Policy Exists

    Wait For Config File   ServerProtectionLinux-Base   ${sulConfigPath}
    Log File  ${sulConfigPath}
    Copy File  ${sulConfigPath}  /tmp/update_config.json.bak
    Replace Sophos URLS to Localhost  ${usingUpdateCache}

    Log File  ${sulConfigPath}

Wait for connection to update cache
    Require SDDS Urls Redirected To Localhost  usingUpdateCache=${True}
    Simulate Update Now
    Wait Until Keyword Succeeds
    ...  30
    ...  5
    ...  Check Suldownloader Log Contains   Successfully connected to: Update cache at localhost:1236

Check ALC Policy Exists
    Wait Until Keyword Succeeds
    ...  60
    ...  1
    ...  File Should Exist   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml


Request Nova To Send Update Now Action
    Remove File  ${MCS_DIR}/status/ALC_status.xml
    Update Now

Wait Nova Report New UpdateSuccess
    [Arguments]   ${Event_Number}
    Wait Until Update Event Has Been Sent   ${Event_Number}
    Check MCS Envelope Contains Event Success On N Event Sent   ${Event_Number}

Check Nova Report Does Not Contain UpdateSuccess For More Than N Events
    [Arguments]   ${Event_Number}
    sleep  20
    Wait Until Update Event Has Been Sent   ${Event_Number}

Wait Nova Report New UpdateFailure
    [Arguments]  ${Event_Number}
    Wait Until Update Event Has Been Sent  ${Event_Number}
    Check MCS Envelope Contains Event Fail On N Event Sent   ${Event_Number}

Wait Nova Report New UpdateReboot
    ${starttime} =  Get Current Epoch Time
    ${endtime} =  Evaluate  ${starttime} + 120
    Get Nova Events  UpdateReboot  ${starttime}  ${endtime}

Get Current Epoch Time
    ${time} =  Get Time
    ${epochTime} =  Convert Date  ${time}  epoch
    [Return]  ${epochTime}

Check For Content In File
    [Arguments]     ${expectedFileContent}     ${pathToCheck}
    # check if mcs file has been created on disk
    @{files} = 	List Files In Directory	   ${pathToCheck}

    ${result} =  Set Variable   False

    ${FILE_CONTENT} =  Set Variable  ''

    :FOR    ${item}     IN      @{files}
    \   ${filecontent} =     Get File    ${pathToCheck}/${item}
    \   Log File    ${pathToCheck}/${item}
    \   ${result} =   Set Variable If   """${expectedFileContent}""" in """${filecontent}"""     True
    \   Run Keyword If   ${result}   Exit For Loop

   Should Be True   ${result}


Check Directory Files With Wait
    [Arguments]  ${timeToTry}  ${timeBetweenTries}  ${expectedFileContent}  ${directory}
    Wait Until Keyword Succeeds
    ...  ${timeToTry}
    ...  ${timeBetweenTries}
    ...  Check For Content In File    ${expectedFileContent}  ${directory}

Wait For Sul Config File To Be Updated From Policy
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  File Should Exist   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

    Wait For Config File   ServerProtectionLinux-Base   ${sulConfigPath}

    Log File   ${sulConfigPath}
    Log File   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml


Log All Files In Directory
    # check if mcs file has been created on disk
    @{files} = 	List Files In Directory	   ${SOPHOS_INSTALL}/base/update/var/

    :FOR    ${item}     IN      @{files}
    \   Log File   ${SOPHOS_INSTALL}/base/update/var/${item}


Wait Until Update Event Has Been Sent
    [Arguments]  ${eventNumber}
    Wait Until Keyword Succeeds
    ...  2 min
    ...  2 secs
    ...  Check Mcs Envelope Log Contains String N Times  <event><appId>ALC</appId>   ${eventNumber}


Check MCS Envelope Does Not Contain Event Success On N Event Sent
    [Arguments]  ${Event_Number}
    Check MCS Envelope Contains Event Fail On N Event Sent   ${Event_Number}

Check MCS Envelope Contains Event Success On N Event Sent
    [Arguments]  ${Event_Number}
    ${string}=  Check Log And Return Nth Occurence Between Strings   <event><appId>ALC</appId>  </event>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Event_Number}
    Should contain   ${string}   &lt;number&gt;0&lt;/number&gt;

Check MCS Envelope Contains Event On N Event Sent
    [Arguments]  ${Event_Number}
    ${string}=  Check Log And Return Nth Occurence Between Strings   <event><appId>ALC</appId>  </event>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Event_Number}

Check MCS Envelope Contains Event Fail On N Event Sent
    [Arguments]  ${Event_Number}
    ${string}=  Check Log And Return Nth Occurence Between Strings   <event><appId>ALC</appId>  </event>  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log  ${Event_Number}
    Should Not Contain   ${string}   &lt;number&gt;0&lt;/number&gt;

Register With Real Update Cache and Message Relay Account
    [Arguments]   ${messageRelayOptions}=
    Set Environment Variable  MCS_CONFIG_SET  ucmr-nova
    Reload Cloud Options
    ${ucmrRegCommand} =  Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Get Registration Command

    Mark All Logs  Registering with Real Update Cache and Message Relay account
    # Remove the file so that we can use the "SulDownloader Reports Finished" function.
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Register With Central  ${ucmrRegCommand} ${messageRelayOptions}

Register With Non MDR Account
    Set Environment Variable  MCS_CONFIG_SET  sav-nova
    Reload Cloud Options
    ${savRegCommand} =  Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Get Registration Command

    Mark All Logs  Registering with non-MDR account
    # Remove the file so that we can use the "SulDownloader Reports Finished" function.
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Register With Central  ${savRegCommand}

Register With MDR Account
    Set Environment Variable  MCS_CONFIG_SET  sspl-nova
    Reload Cloud Options
    ${ssplRegCommand} =  Wait Until Keyword Succeeds
    ...  30 secs
    ...  10 secs
    ...  Get Registration Command

    Mark All Logs  Registering with MDR account
    # Remove the file so that we can use the "SulDownloader Reports Finished" function.
    Remove File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    Register With Central  ${ssplRegCommand}

Wait For ALC Policy To Not Contain MDR
    Wait Until Keyword Succeeds
    ...  60
    ...  1
    ...  File Should Exist   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

    Check ALC Policy Does Not Contain MDR

Check ALC Policy Does Not Contain MDR
    ${alcPolicy} =  Get File  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    Should Not Contain  ${alcPolicy}   MDR


Wait For ALC Policy To Contain MDR
    Wait Until Keyword Succeeds
    ...  60
    ...  1
    ...  File Should Exist   ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml

    Check ALC Policy Contains MDR

Check ALC Policy Contains MDR
    ${alcPolicy} =  Get File  ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    Should Contain  ${alcPolicy}   MDR


Wait For MCS Router To Be Running
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  Check MCS Router Running

Wait For MDR Status
    Wait Until Keyword Succeeds
    ...  30
    ...  1
    ...  File Should Exist  ${SOPHOS_INSTALL}/base/mcs/status/MDR_status.xml

Wait For MDR To Be Installed
    Wait Until Keyword Succeeds
    ...  60
    ...  1
    ...  Check MDR Component Suite Installed Correctly


Nova Suite Teardown
    Dump Appserver Log
    Uninstall SSPL Unless Cleanup Disabled
    Revert Hostfile
    Run Keyword And Ignore Error   Log File  /tmp/registerCommand
    Remove File  /tmp/registerCommand

Nova Test Teardown
    [Arguments]  ${requireDeRegister}=False
    Run Keyword If Test Failed     Dump Appserver Log
    MCSRouter Test Teardown
    Dump Logs And Clean Up Temp Dir
    Run Keyword If  ${requireDeRegister}   Deregister From Central
    Run Keyword If Test Failed    Reset Environment For Nova Tests


Setup MCS Tests Nova
    Setup Host File
    Require Fresh Install
    Remove Environment Variable  MCS_CONFIG_SET
    Reload Cloud Options
    Set Default Credentials
    Set Nova MCS CA Environment Variable
    ${regCommand}=  Get Sspl Registration
    Set Suite Variable    ${regCommand}     ${regCommand}   children=true

Setup Real Update Cache And Message Relay Tests With Nova
    Setup Host File
    Require Fresh Install
    Set Nova MCS CA Environment Variable
    ${regCommand}=  Get Sspl Registration
    Set Suite Variable    ${regCommand}     ${regCommand}   children=true

Reset Environment For Nova Tests
    Nova Suite Teardown
    Setup MCS Tests Nova

