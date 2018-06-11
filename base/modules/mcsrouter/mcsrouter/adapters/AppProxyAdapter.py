#!/usr/bin/env python

from __future__ import print_function,division,unicode_literals

import xml.dom.minidom

import logging
logger = logging.getLogger(__name__)

import AdapterBase

import utils.Timestamp
import utils.XmlHelper
import mcsclient.MCSCommands

class AppProxyAdapter(AdapterBase.AdapterBase):
    def __init__(self, appids):
        self.__m_appids = appids

    def getAppId(self):
        return "APPSPROXY"

    def getStatusXml(self):
        statusxml = []

        statusxml.append('<?xml version="1.0" encoding="utf-8"?>')
        statusxml.append('<ns:registeredApplicationsStatus xmlns:ns="http://www.sophos.com/xml/mcs/registeredApplicationsStatus">')
        statusxml.append('<meta protocolVersion="1.0" timestamp="%s"/>'%(utils.Timestamp.timestamp()))
        for appid in self.__m_appids:
            statusxml.append('<application name="%s"/>'%appid)
        statusxml.append('</ns:registeredApplicationsStatus>')

        return "".join(statusxml)

    def _hasNewStatus(self):
        return False

    def processCommand(self, command):
        """
        Process a command object.

        body =
        "
<?xml version="1.0" encoding="utf-8"?>
<ns:policyAssignments xmlns:ns="http://www.sophos.com/xml/mcs/policyAssignments">
    <meta protocolVersion="1.0"/>
    <policyAssignment>
        <appId>SAV</appId>
        <policyId>9db6354d-c5e4-4af4-b512-a1f5860f419a</policyId>
    </policyAssignment>
</ns:policyAssignments>
        "

        {u'body':
u'<?xml version="1.0"?>
<ns:policyFragments xmlns:ns="http://www.sophos.com/xml/mcs/policyFragments">
    <meta protocolVersion="1.0"/>
    <fragments>
        <appId>HBT</appId>
        <fragment seq="0" id="248a51ecac446bdfa3ff56d8f2bb8ad64d29ffa142786816dcade032782483a4"/>
        <fragment seq="1" id="e6da4cf3ebbc0797010174b613b10f59c00d9aa49822a45dc70929c9bc5db003"/>
        <fragment seq="2" id="77e46fdd793bb1a4e442ee0149170619b0f275759e342f7c63138a442d2d3891"/>
    </fragments>
</ns:policyFragments>', u'seq': u'3', u'appId': u'APPSPROXY', u'creationTime': u'2016-09-21T14:19:02.777Z', u'ttl': u'PT10000S', u'id': u'2811'}

        @return list of commands to process
        """
        connection = command.getConnection()
        body = command.get("body")

        try:
            doc = xml.dom.minidom.parseString(body)
        except Exception:
            logger.exception("Unable to parse AppProxy Action: '%s' from '%s'", body, command.getXmlText())
            return []

        try:
            policyAssignments = doc.getElementsByTagName("policyAssignment")
            commands = []
            logger.debug("Received %d policyAssignments", len(policyAssignments))
            commandID = command.get('id')

            for policyAssignment in policyAssignments:
                appId = utils.XmlHelper.getTextNodeText(policyAssignment, "appId")
                policyId = utils.XmlHelper.getTextNodeText(policyAssignment, "policyId")
                commands.append(mcsclient.MCSCommands.PolicyCommand(
                        commandID, appId, policyId, connection))

            fragmentedAssignments = doc.getElementsByTagName("fragments")
            logger.debug("Received %d fragmented policy assignments", len(fragmentedAssignments))

            for fragmented in fragmentedAssignments:
                appId = utils.XmlHelper.getTextNodeText(fragmented, "appId")
                fragmentNodes = fragmented.getElementsByTagName("fragment")
                commands.append(mcsclient.MCSCommands.FragmentedPolicyCommand(
                        commandID, appId, fragmentNodes, connection))
        finally:
            doc.unlink()
            command.complete()

        return commands
