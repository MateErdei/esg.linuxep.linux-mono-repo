*** Settings ***
Suite Setup      Setup SUL Downloader Tests
Suite Teardown   Cleanup SUL Downloader Tests


Library    Process
Library    OperatingSystem
Library    ${LIBS_DIRECTORY}/SulDownloader.py
Library    ${LIBS_DIRECTORY}/FullInstallerUtils.py

*** Keywords ***

### Setup
Setup SUL Downloader Tests
    Run Full Installer
    File Should Exist  ${SUL_DOWNLOADER}
    File Should Exist  ${VERSIGPATH}

### Cleanup
Cleanup SUL Downloader Tests
    No Operation
    Uninstall SSPL
