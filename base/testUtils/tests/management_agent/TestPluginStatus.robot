*** Settings ***
Documentation     Test the Management Agent

Library           Process
Library           OperatingSystem
Library           Collections

Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/FakePluginWrapper.py


Resource    ManagementAgentResources.robot

Default Tags    MANAGEMENT_AGENT
*** Test Cases ***
Verify Management Agent Can Receive Plugin Status
    [Tags]  SMOKE  MANAGEMENT_AGENT
    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry
    Start Management Agent

    Start Plugin

    ${content}    Evaluate    str('content1')
    Send Plugin Status   ${content}

    Check Status File   ${content}

    # clean up
    Stop Plugin
    Stop Management Agent


Verify Management Agent Can Receive Plugin Status When Plugin Already Started

    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry
    Start Plugin

    Start Management Agent

    ${content}    Evaluate    str('content')
    Send Plugin Status   ${content}
    Check Status File   ${content}

    # clean up
    Stop Plugin
    Stop Management Agent



Verify Management Agent Can Receive Plugin Status When Management Agent is Restarted

    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry

    # Start Management agent and fake plugin and send status, and check output

    Start Management Agent

    Start Plugin

    ${content}    Evaluate    str('content1')
    Send Plugin Status   ${content}
    Check Status File   ${content}

    # Restart management agent, and send new status message and check output

    Stop Management Agent
    Wait For Management Agent To Stop
    Start Management Agent

    ${content}    Evaluate    str('content2')
    Send Plugin Status   ${content}
    Check Status File   ${content}

    # clean up
    Stop Plugin
    Stop Management Agent


Verify Management Agent Can Process Plugin Status When Management Agent is Restarted After Status Changed

    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry

    # Start Management agent and fake plugin and send status, and check output

    Start Management Agent

    Start Plugin

    ${content}    Evaluate    str('content1')
    Send Plugin Status   ${content}
    Check Status File   ${content}

    # Update Status for plugin while Management Agent is not running
    # Restart management agent, and check new Status is sent.

    Stop Management Agent

    Wait For Management Agent To Stop

    ${content}    Evaluate    str('content2')
    Send Plugin Status    ${content}

    Start Management Agent

    Check Status File   ${content}

    # clean up
    Stop Plugin
    Stop Management Agent


Verify Management Agent Writes To Status Cache

    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry

    # Start Management agent and fake plugin and send status, and check output

    Start Management Agent

    Start Plugin

    ${content}    Evaluate    str('content1')
    Send Plugin Status   ${content}
    Check Status File   ${content}
    Check Status Cache File   ${content}

    # Update Status for plugin while Management Agent is not running
    # Restart management agent, and check new Status is sent.

    Stop Management Agent
    Wait For Management Agent To Stop

    ${content}    Evaluate    str('content2')
    Send Plugin Status    ${content}

    Start Management Agent

    Check Status File   ${content}
    Check Status Cache File   ${content}

    # clean up
    Stop Plugin
    Stop Management Agent


Verify Management Agent Only Updates Status And Status Cache When Status Content Changes

    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry

    # Start Management agent and fake plugin and send status, and check output

    Start Management Agent

    Start Plugin

    ${content1}    Evaluate    str('content1')
    Send Plugin Status   ${content1}
    Check Status File   ${content1}
    Check Status Cache File   ${content1}

    ${statusCacheTimeOrig}    Get Modified Time   ${SOPHOS_INSTALL}/base/mcs/status/cache/SAV.xml
    ${statusTimeOrig}         Get Modified Time   ${SOPHOS_INSTALL}/base/mcs/status/SAV_status.xml

    #Make sure the time stamp would change on the file check
    Sleep   2

    Send Plugin Status   ${content1}
    Check Status File   ${content1}
    Check Status Cache File   ${content1}

    ${statusCacheTimeActual}    Get Modified Time   ${SOPHOS_INSTALL}/base/mcs/status/cache/SAV.xml
    ${statusTimeActual}         Get Modified Time   ${SOPHOS_INSTALL}/base/mcs/status/SAV_status.xml

    Should Be True   """${statusCacheTimeOrig}""" == """${statusCacheTimeActual}"""
    Should Be True   """${statusTimeOrig}""" == """${statusTimeActual}"""

    #Make sure the time stamp would change on the file check
    Sleep   2

    ${content2}    Evaluate    str('content2')
    Send Plugin Status   ${content2}
    Check Status File   ${content2}
    Check Status Cache File   ${content2}


    ${statusCacheTimeActual}    Get Modified Time   ${SOPHOS_INSTALL}/base/mcs/status/cache/SAV.xml
    ${statusTimeActual}         Get Modified Time   ${SOPHOS_INSTALL}/base/mcs/status/SAV_status.xml

    Should Be True   """${statusCacheTimeOrig}""" != """${statusCacheTimeActual}"""
    Should Be True   """${statusTimeOrig}""" != """${statusTimeActual}"""

Verify Management Agent Loads Status Cache On Startup

    # make sure no previous status xml file exists.
    Remove Status Xml Files

    Setup Plugin Registry

    #Create Status Cache
    ${content1}    Evaluate    str('content1')
    Create File     ${SOPHOS_INSTALL}/base/mcs/status/cache/SAV.xml   ${content1}
    Create File     ${SOPHOS_INSTALL}/base/mcs/status/SAV_status.xml  ${content1}

    ${statusCacheTimeOrig}    Get Modified Time   ${SOPHOS_INSTALL}/base/mcs/status/cache/SAV.xml
    ${statusTimeOrig}         Get Modified Time   ${SOPHOS_INSTALL}/base/mcs/status/SAV_status.xml

    # Start Management agent and fake plugin and send status, and check output

    Start Management Agent

    Start Plugin

    ${content1}    Evaluate    str('content1')
    Send Plugin Status   ${content1}
    Check Status File   ${content1}
    Check Status Cache File   ${content1}

    ${statusCacheTimeActual}    Get Modified Time   ${SOPHOS_INSTALL}/base/mcs/status/cache/SAV.xml
    ${statusTimeActual}         Get Modified Time   ${SOPHOS_INSTALL}/base/mcs/status/SAV_status.xml

    Should Be True   """${statusCacheTimeOrig}""" == """${statusCacheTimeActual}"""
    Should Be True   """${statusTimeOrig}""" == """${statusTimeActual}"""

    #Make sure the time stamp would change on the file check
    Sleep   2

    ${content2}    Evaluate    str('content2')
    Send Plugin Status   ${content2}
    Check Status File   ${content2}
    Check Status Cache File   ${content2}


    ${statusCacheTimeActual}    Get Modified Time   ${SOPHOS_INSTALL}/base/mcs/status/cache/SAV.xml
    ${statusTimeActual}         Get Modified Time   ${SOPHOS_INSTALL}/base/mcs/status/SAV_status.xml

    Should Be True   """${statusCacheTimeOrig}""" != """${statusCacheTimeActual}"""
    Should Be True   """${statusTimeOrig}""" != """${statusTimeActual}"""







