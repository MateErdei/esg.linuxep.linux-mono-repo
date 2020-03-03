*** Settings ***
Documentation    Updating from Ostia

Library         ../Libs/WarehouseUtils.py

Resource        ../shared/AVResources.robot


*** Keywords ***
Setup Ostia Warehouse Environment
    Generate Local Ssl Certs If They Dont Exist
    Install Local SSL Server Cert To System
    Install Ostia SSL Certs To System
    Setup Local Warehouses If Needed


Install Just Base
    Remove Directory   ${SOPHOS_INSTALL}   recursive=True
    Directory Should Not Exist  ${SOPHOS_INSTALL}
    Install Base For Component Tests

Create Policy For Updating From Ostia
    [Arguments]  ${policy_path}
    ${policy} =  WarehouseUtils.get warehouse policy from template  ${policy_path}
    [return]  ${policy}


Apply Policy to Base
    [Arguments]  ${policy_xml}
    Log  Got policy XML ${policy_xml}


Trigger Update
    Log  Trigger Update


Verify AV installed
    Log  Verify AV installed


*** Test Cases ***

Update from Ostia
    Install Just Base
    ${policy} =  Create Policy For Updating From Ostia  ${RESOURCES_PATH}/alc_policy/template/base_and_av_VUT.xml
    Apply Policy to Base  ${policy}
    Trigger Update
    Verify AV installed
#
#
#    Set Local CA Environment Variable
#    Setup Ostia Warehouse Environment
#

