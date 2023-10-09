*** Settings ***
Resource    ../update/SDDS3Resources.robot


Suite Setup      Setup SUL Downloader Tests
Suite Teardown   Cleanup SUL Downloader Tests


Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/SulDownloader.py
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py


*** Variables ***

${SUL_DOWNLOADER_LOG} =  ${SOPHOS_INSTALL}/logs/base/suldownloader.log

*** Keywords ***

### Setup
Setup SUL Downloader Tests
    ${base_package}=    Generate Local Base SDDS3 Package
    Set Global Variable    ${sdds3_base_package}    ${base_package}
    Run Full Installer
    File Should Exist  ${SUL_DOWNLOADER}
    File Should Exist  ${VERSIGPATH}

### Cleanup
Cleanup SUL Downloader Tests
    Uninstall SSPL
    Clean up local Base SDDS3 Package
