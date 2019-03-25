"""
XMLHelper Module
"""

import xml.dom
import codecs


def get_text_from_element(element):
    """
    get_text_from_element
    """
    text = []
    for child_node in element.childNodes:
        if child_node.nodeType == xml.dom.Node.TEXT_NODE:
            text.append(child_node.data)
    return "".join(text)


def get_text_node_text(node, element_name):
    """
    get_text_node_text
    """
    element = node.getElementsByTagName(element_name)[0]
    return get_text_from_element(element)


def get_escaped_non_ascii_content(file_path):
    """
    get_escaped_non_ascii_content
    """
    body = codecs.open(
        file_path,
        encoding='utf-8',
        mode='r',
        errors='replace').read()
    return body.encode('ascii', 'xmlcharrefreplace')
