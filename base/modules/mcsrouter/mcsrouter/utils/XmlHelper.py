
import xml.dom
import codecs


def get_text_from_element(element):
    text = []
    for n in element.childNodes:
        if n.nodeType == xml.dom.Node.TEXT_NODE:
            text.append(n.data)
    return "".join(text)


def get_text_node_text(node, element_name):
    element = node.getElementsByTagName(element_name)[0]
    return get_text_from_element(element)


def get_xml_file_content_with_escaped_non_ascii_code(file_path):
    body = codecs.open(
        file_path,
        encoding='utf-8',
        mode='r',
        errors='replace').read()
    return body.encode('ascii', 'xmlcharrefreplace')
