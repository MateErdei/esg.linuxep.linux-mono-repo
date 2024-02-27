*** Settings ***
Library   BuiltIn
Library   ${COMMON_TEST_LIBS}/IniUtils.py


*** Test Cases ***
Example Test
    Log To Console  Hello, world!
    ${basever}=  Get Version Number From INI File   /opt/sophos-spl/base/VERSION.ini
    Log To Console  Base version: ${basever}
