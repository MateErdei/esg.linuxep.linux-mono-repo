*** Settings ***
Resource  TemplatePluginResources.robot

*** Test Cases ***
Example Test
    Example Keyword
    File Should Contain  /tmp/examplefile.txt  "Contents"

*** Keywords ***
Example Keyword In Test Suite
    Create File  /tmp/examplefile2.txt  "Contents"






