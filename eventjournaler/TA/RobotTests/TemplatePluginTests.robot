*** Settings ***
Resource  TemplatePluginResources.robot

*** Test Cases ***
Example Test
    Example Keyword In Test Suite
    File Should Contain  /tmp/examplefile.txt  "Contents"

Plugin Template Installs
    Run Shell Process  bash -x ${TEMPLATE_PLUGIN_SDDS}/install.sh 2> /tmp/installer.log   OnError=Failed to Install Template Plugin   timeout=60s
    Directory Should Exist  ${SOPHOS_INSTALL}/plugins/TemplatePlugin

*** Keywords ***
Example Keyword In Test Suite
    Create File  /tmp/examplefile2.txt  "Contents"






