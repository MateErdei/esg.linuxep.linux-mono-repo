#!/usr/bin/env python
"""
app_proxy_adapter Module
"""

from __future__ import print_function, division, unicode_literals

import xml.dom.minidom
import logging

import mcsrouter.adapters.adapter_base
import mcsrouter.utils.timestamp
import mcsrouter.utils.xml_helper
import mcsrouter.mcsclient.mcs_commands

LOGGER = logging.getLogger(__name__)


class AppProxyAdapter(mcsrouter.adapters.adapter_base.AdapterBase):
    """
    AppProxyAdapter class
    """

    def __init__(self, app_ids):
        """
        __init__
        """
        self.__m_app_ids = app_ids

    def get_app_id(self):
        """
        get_app_id
        """
        return "APPSPROXY"

    def get_status_xml(self):
        """
        get_status_xml
        """
        status_xml = []

        status_xml.append('<?xml version="1.0" encoding="utf-8"?>')
        status_xml.append(
            '<ns:registeredApplicationsStatus xmlns:ns="http://www.sophos.com/xml/mcs/registeredApplicationsStatus">')
        status_xml.append(
            '<meta protocolVersion="1.0" timestamp="%s"/>' %
            (mcsrouter.utils.timestamp.timestamp()))
        for app_id in self.__m_app_ids:
            status_xml.append('<application name="%s"/>' % app_id)
        status_xml.append('</ns:registeredApplicationsStatus>')

        return "".join(status_xml)

    def _has_new_status(self):
        """
        _has_new_status
        """
        return False

    def process_command(self, command):
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
        connection = command.get_connection()
        body = command.get("body")

        try:
            doc = xml.dom.minidom.parseString(body)
        except Exception:
            LOGGER.exception(
                "Unable to parse AppProxy Action: '%s' from '%s'",
                body,
                command.get_xml_text())
            return []

        try:
            policy_assignments = doc.getElementsByTagName("policyAssignment")
            commands = []
            LOGGER.debug(
                "Received %d policyAssignments",
                len(policy_assignments))
            command_id = command.get('id')

            for policy_assignment in policy_assignments:
                app_id = mcsrouter.utils.xml_helper.get_text_node_text(
                    policy_assignment, "appId")
                policy_id = mcsrouter.utils.xml_helper.get_text_node_text(
                    policy_assignment, "policyId")
                commands.append(mcsrouter.mcsclient.mcs_commands.PolicyCommand(
                    command_id, app_id, policy_id, connection))

            fragmented_assignments = doc.getElementsByTagName("fragments")
            LOGGER.debug(
                "Received %d fragmented policy assignments",
                len(fragmented_assignments))

            for fragmented in fragmented_assignments:
                app_id = mcsrouter.utils.xml_helper.get_text_node_text(
                    fragmented, "appId")
                fragment_nodes = fragmented.getElementsByTagName("fragment")
                commands.append(
                    mcsrouter.mcsclient.mcs_commands.FragmentedPolicyCommand(
                        command_id, app_id, fragment_nodes, connection))
        finally:
            doc.unlink()
            command.complete()

        return commands
