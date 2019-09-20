# Copyright 2019 Sophos Plc, Oxford, England.
"""
adapter_base Module
"""

import logging
import xml.dom.minidom

import mcsrouter.utils.xml_helper
from mcsrouter.utils.xml_helper import toxml_utf8

LOGGER = logging.getLogger(__name__)

TEMPLATE_STATUS_AND_CONFIGURATION_XML = """<?xml version='1.0' encoding='utf-8'?>
<StatusAndConfig>
<status>
</status>
<configuration>
</configuration>
</StatusAndConfig>
"""


def remove_blanks(node):
    """
    remove_blanks
    """
    for child_node in node.childNodes:
        if child_node.nodeType == xml.dom.Node.TEXT_NODE:
            if child_node.nodeValue:
                child_node.nodeValue = child_node.nodeValue.strip()
        elif child_node.nodeType == xml.dom.Node.ELEMENT_NODE:
            remove_blanks(child_node)


class AdapterBase:
    """
    AdapterBase class
    """
    # pylint: disable=no-self-use

    def _parse_xml_string(self, content):
        return mcsrouter.utils.xml_helper.parseString(content)

    def get_status_ttl(self):
        """
        get_status_ttl
        """
        return 10000

    def _set_text(self, node, doc, value):
        """
        _set_text
        """
        assert isinstance(value, str)
        text = doc.createTextNode(value)
        for child in node.childNodes:
            node.removeChild(child)
        node.appendChild(text)

    def _set_boolean(self, node, doc, value):
        """
        _set_boolean
        """
        if value:
            self._set_text(node, doc, "true")
        else:
            self._set_text(node, doc, "false")

    def _text_node(self, doc, name, value):
        """
        _text_node
        """
        element = doc.createElement(name)
        text = doc.createTextNode(value)
        element.appendChild(text)
        return element

    def _add_text_node(self, doc, parent_node, node_name, node_value):
        """
        _add_text_node
        """
        node = self._text_node(doc, node_name, node_value)
        parent_node.appendChild(node)

    def has_new_status(self):
        """
        has_new_status
        """
        return self._has_new_status()

    def _has_new_status(self):
        """
        _has_new_status
        """
        pass

    def _get_status_xml(self):
        """
        _get_status_xml
        """
        return ""

    def get_status_xml(self):
        """
        get_status_xml
        """
        return self.get_status_and_config_xml()

    def get_status_and_config_xml(self):
        """
        get_status_and_config_xml
        """
        status = self._get_status_xml()
        if status is None:
            return None
        assert isinstance(status, str)

        doc = self._parse_xml_string(
            TEMPLATE_STATUS_AND_CONFIGURATION_XML)
        remove_blanks(doc)
        status_node = doc.getElementsByTagName("status")[0]

        self._set_text(status_node, doc, status)
        config_node = doc.getElementsByTagName("configuration")[0]
        conf_xml = self._get_config_xml()
        self._set_text(config_node, doc, conf_xml)

        output = toxml_utf8(doc)
        doc.unlink()
        return output

    def _get_config_xml(self):
        """
        _get_config_xml
        """
        return ""

    def process_log_event(self):
        """
        process_log_event
        """
        return None
