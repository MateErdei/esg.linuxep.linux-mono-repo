
import xml.dom
import codecs

def getTextFromElement(element):
    text = []
    for n in element.childNodes:
        if n.nodeType == xml.dom.Node.TEXT_NODE:
            text.append(n.data)
    return "".join(text)

def getTextNodeText(node, elementName):
    element = node.getElementsByTagName(elementName)[0]
    return getTextFromElement(element)

def getXMLfileContentWithEscapedNonAsciiCode( filepath ):
    body = codecs.open(filepath, encoding='utf-8', mode='r', errors='replace').read()
    return body.encode('ascii', 'xmlcharrefreplace')
