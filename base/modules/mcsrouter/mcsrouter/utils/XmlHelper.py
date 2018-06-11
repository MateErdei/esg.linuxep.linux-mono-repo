
import xml.dom

def getTextFromElement(element):
    text = []
    for n in element.childNodes:
        if n.nodeType == xml.dom.Node.TEXT_NODE:
            text.append(n.data)
    return "".join(text)

def getTextNodeText(node, elementName):
    element = node.getElementsByTagName(elementName)[0]
    return getTextFromElement(element)
