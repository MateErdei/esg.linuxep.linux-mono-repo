
import xml.dom.minidom

import logging
logger = logging.getLogger(__name__)

TEMPLATE_STATUS_AND_CONFIGURATION_XML = """<?xml version='1.0' encoding='utf-8'?>
<StatusAndConfig>
<status>
</status>
<configuration>
</configuration>
</StatusAndConfig>
"""


def remove_blanks(node):
    for child_node in node.childNodes:
        if child_node.nodeType == xml.dom.Node.TEXT_NODE:
            if child_node.nodeValue:
                child_node.nodeValue = child_node.nodeValue.strip()
        elif child_node.nodeType == xml.dom.Node.ELEMENT_NODE:
            remove_blanks(child_node)


class AdapterBase(object):
    def get_status_ttl(self):
        return 10000

    def _set_text(self, node, doc, value):
        text = doc.createTextNode(value)
        for child in node.childNodes:
            node.removeChild(child)
        node.appendChild(text)

    def _set_boolean(self, node, doc, value):
        if value:
            self._set_text(node, doc, "true")
        else:
            self._set_text(node, doc, "false")

    def _text_node(self, doc, name, value):
        element = doc.createElement(name)
        text = doc.createTextNode(value)
        element.appendChild(text)
        return element

    def _add_text_node(self, doc, parent_node, node_name, node_value):
        node = self._text_node(doc, node_name, node_value)
        parent_node.appendChild(node)

    def has_new_status(self):
        return self._has_new_status()

    def get_status_xml(self):
        return self.get_status_and_config_xml()

    def get_status_and_config_xml(self):
        status = self._get_status_xml()

        if status is None:
            return None

        doc = xml.dom.minidom.parseString(
            TEMPLATE_STATUS_AND_CONFIGURATION_XML)
        remove_blanks(doc)
        status_node = doc.getElementsByTagName("status")[0]
        self._set_text(status_node, doc, status)
        config_node = doc.getElementsByTagName("configuration")[0]
        self._set_text(config_node, doc, self._get_config_xml())

        output = doc.toxml(encoding="utf-8")
        doc.unlink()
        return output

    def _get_config_xml(self):
        return ""

    def process_log_event(self):
        return None
