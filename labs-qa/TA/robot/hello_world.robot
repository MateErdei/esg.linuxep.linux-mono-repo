*** Settings ***
Library   BuiltIn
Library   ${COMMON_TEST_LIBS}/IniUtils.py
Library   ../libs/VersionInfo.py


*** Test Cases ***
Example Test
    Log To Console    Hello, world!
    ${basever}=    Get Version Number From INI File    /opt/sophos-spl/base/VERSION.ini
    Log To Console    Product base version: ${basever}
    ${versions_to_present} =    Create List    SFS_Data    SFS_Engine    ML_Data
    &{versions}=    Get SUSI Configuration    ${versions_to_present}
    Log To Console    Product Configuration:
    Log To Console    VDB version: ${versions}[SFS_Data]
    Log To Console    Engine version: ${versions}[SFS_Engine]
    Log To Console    ML data version: ${versions}[ML_Data]