*** Settings ***
Library   BuiltIn
Library   ${COMMON_TEST_LIBS}/IniUtils.py
Library   ../VersionInfo.py


*** Test Cases ***
Example Test
    Log To Console  Hello, world!
    ${basever}=  Get Version Number From INI File   /opt/sophos-spl/base/VERSION.ini
    Log To Console  Product base version: ${basever}
    &{versions}=  Get VDB and Engine Versions
    Log To Console  VDB version: ${versions}[VDB]
    Log To Console  Engine version: ${versions}[Engine]