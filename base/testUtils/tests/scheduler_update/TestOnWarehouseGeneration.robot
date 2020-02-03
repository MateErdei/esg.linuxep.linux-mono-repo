*** Settings ***
Documentation    Check Requirements on Warehouse generation

Test Setup      Setup For Test
Test Teardown   Teardown For Test

Resource  ../GeneralTeardownResource.robot

Library    Process
Library    OperatingSystem
Library    ${libs_directory}/UpdateServer.py
Library    ${libs_directory}/WarehouseGenerator.py
Library    ${libs_directory}/LogUtils.py
Library    ${libs_directory}/FullInstallerUtils.py

Default Tags   SULDOWNLOADER

*** Variables ***
${tmpdir}                       ${SOPHOS_INSTALL}/tmp/SDT

*** Test Cases ***
Warehouse Generates ReleaseTag with BaseVersion Associated
    Generate Update Certs
    ${dist} =  Get Folder With Installer

    Copy Directory  ${dist}  ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Base

    Add Component Warehouse Config   ServerProtectionLinux-Base   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ServerProtectionLinux-Base
    Generate Warehouse

    ${mainconfig} =  Get File  ${tmpdir}/temp_warehouse/sdds/mainconfig.xml

    Should Contain  ${mainconfig}  </Tag><Base>


Warehouse Can be configure to Not Generate ReleaseTag with BaseVersion Associated
    Generate Update Certs
    ${dist} =  Get Folder With Installer

    Copy Directory  ${dist}  ${tmpdir}/TestInstallFiles/ServerProtectionLinux-Base

    Add Component Warehouse Config   ServerProtectionLinux-Base   ${tmpdir}/TestInstallFiles/    ${tmpdir}/temp_warehouse/   ServerProtectionLinux-Base
    Generate Warehouse     RemoveBaseVersion=True

    ${mainconfig} =  Get File  ${tmpdir}/temp_warehouse/sdds/mainconfig.xml

    Should Contain  ${mainconfig}  <ReleaseTag><Tag>RECOMMENDED</Tag></ReleaseTag>
    Should Not Contain  ${mainconfig}  </Tag><Base>


*** Keywords ***
Setup For Test
    Create Directory    ${tmpdir}

Teardown For Test
    General Test Teardown
    Remove Directory   ${tmpdir}    recursive=True

