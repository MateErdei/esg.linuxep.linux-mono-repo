*** Settings ***
Library     ${COMMON_TEST_LIBS}/UpdateServer.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py

Force Tags    MANUAL
Test Timeout    NONE

*** Test Case ***
#Following entries are not tests and just start the specified server. Example:
#robot --suite TestUpdateServers - test "Run Basic Auth Proxy Server For 10 Minutes" update_server

Run Basic Auth Proxy Server For 10 Minutes
    Start Proxy Server With Basic Auth    8192    username    password
    Sleep    10 minutes
    
Run Simple Proxy Server For 10 Minutes
    Start Simple Proxy Server    8192
    Sleep    10 minutes

Run Digest Auth Proxy Server For 10 Minutes
    Start Proxy Server With Digest Auth    8192    username    password
    Sleep    10 minutes

Run Update Server For 10 Minutes
    Start Update Server    8192    ${SUPPORT_FILES}/update_cache
    Sleep    10 minutes
