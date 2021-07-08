*** Settings ***
Resource  TemplatePluginResources.robot

*** Test Cases ***
Example Test
    Example Keyword
    File Should Contain  /tmp/examplefile.txt  "Contents"

Plugin Template Installs
    Create Directory  ${SOPHOS_INSTALL}
    Run Shell Process   bash -x ${BASE_SDDS}/install.sh 2> /tmp/installer.log   OnError=Failed to Install Base   timeout=60s
    Run Keyword and Ignore Error   Run Shell Process    /opt/sophos-spl/bin/wdctl stop mcsrouter  OnError=Failed to stop mcsrouter
    Run Shell Process   bash -x ${TEMPLATE_PLUGIN_SDDS}/install.sh 2> /tmp/installer.log   OnError=Failed to Install Template Plugin   timeout=60s
    Directory Should Exist  ${SOPHOS_INSTALL}/plugins/TemplatePlugin
    Run Process  bash ${SOPHOS_INSTALL}/bin/uninstall.sh --force   shell=True   timeout=30s

*** Keywords ***
Example Keyword In Test Suite
    Create File  /tmp/examplefile2.txt  "Contents"






