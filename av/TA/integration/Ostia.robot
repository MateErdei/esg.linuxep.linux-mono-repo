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

#Create Policy For Updating From Ostia


*** Test Cases ***

#Update from Ostia
#    Install Just Base
#    ${policy} =  Create Policy For Updating From Ostia
#    Apply Policy to Base  ${policy}
#    Trigger Update
#    Verify AV installed
#
#
#    Set Local CA Environment Variable
#    Setup Ostia Warehouse Environment
#

