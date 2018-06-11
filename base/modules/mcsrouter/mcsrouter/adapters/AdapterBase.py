
import xml.dom.minidom

import logging
logger = logging.getLogger(__name__)

TEMPLATE_STATUS_AND_CONFIGURATION_XML="""<?xml version='1.0' encoding='utf-8'?>
<StatusAndConfig>
<status>
</status>
<configuration>
</configuration>
</StatusAndConfig>
"""

def remove_blanks(node):
    for x in node.childNodes:
        if x.nodeType == xml.dom.Node.TEXT_NODE:
            if x.nodeValue:
                x.nodeValue = x.nodeValue.strip()
        elif x.nodeType == xml.dom.Node.ELEMENT_NODE:
            remove_blanks(x)

class AdapterBase(object):
    def getStatusTTL(self):
        return 10000

    def _setText(self, node, doc, value):
        text = doc.createTextNode(value)
        for child in node.childNodes:
            node.removeChild(child)
        node.appendChild(text)

    def _setBoolean(self, node, doc, value):
        if value:
            self._setText(node,doc,"true")
        else:
            self._setText(node,doc,"false")


    def _textNode(self, doc, name, value):
        element = doc.createElement(name)
        text = doc.createTextNode(value)
        element.appendChild(text)
        return element

    def _addTextNode(self, doc, parentNode, nodeName, nodeValue):
        node = self._textNode(doc, nodeName, nodeValue)
        parentNode.appendChild(node)

    def hasNewStatus(self):
        return self._hasNewStatus()

    def getStatusXml(self):
        return self._getStatusAndConfigXml()

    def _getStatusAndConfigXml(self):
        status = self._getStatusXml()

        if status is None:
            return None

        doc = xml.dom.minidom.parseString(TEMPLATE_STATUS_AND_CONFIGURATION_XML)
        remove_blanks(doc)
        statusNode = doc.getElementsByTagName("status")[0]
        self._setText(statusNode, doc, status)
        configNode = doc.getElementsByTagName("configuration")[0]
        self._setText(configNode, doc, self._getConfigXml())

        output = doc.toxml(encoding="utf-8")
        doc.unlink()
        #~ logger.warning("_getStatusAndConfigXml:%s",output)
        return output

    def _getConfigXml(self):
        return ""

    def processLogEvent(self, logevent):
        return None

