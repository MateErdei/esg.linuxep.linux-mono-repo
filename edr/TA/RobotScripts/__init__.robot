*** Settings ***
Suite Setup   Global Setup Tasks

Library         OperatingSystem

Test Timeout    5 minutes

*** Keywords ***
Global Setup Tasks
    # SOPHOS_INSTALL
    ${placeholder} =  Get Environment Variable  SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}  ${placeholder}