#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
XMLHelper Module
"""

import xml.dom
import xml.dom.expatbuilder
import xml.dom.xmlbuilder
import xml.parsers.expat
import codecs

import logging
LOGGER = logging.getLogger("mcsrouter")

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
    return body

class NoEntitiesAllowedException(xml.parsers.expat.ExpatError):
    """
    A sub-class of xml.parsers.expat.ExpatError so that we get caught in all the same places
    """
    pass

class XMLException(RuntimeError):
    """
    XMLException class
    """
    pass

class NoEntityExpatBuilderNS(xml.dom.expatbuilder.ExpatBuilderNS):
    """
    NoEntityExpatBuilderNS Class
    """
    def entity_decl_handler(self, entityName, is_parameter_entity, value,
                            base, systemId, publicId, notationName):
        raise NoEntitiesAllowedException("Refusing to parse Entity Declaration: "+entityName)

def parseString(contents):
    options = xml.dom.xmlbuilder.Options()
    options.entities = True
    options.external_parameter_entities = False
    options.external_general_entities = False
    options.external_dtd_subset = False
    options.create_entity_ref_nodes = False
    options.comments = False
    builder = NoEntityExpatBuilderNS(options)
    return builder.parseString(contents)

def check_xml_has_no_script_tags(xml_string):
    if any(x in xml_string for x in ["<xhtml:script", "<script"]):
        err_msg = "Refusing to parse Script Element"
        LOGGER.warning(err_msg)
        raise XMLException(err_msg)

def check_string_length_for_statuses(contents):
    max_size = 256 * 1024  # maximum number of characters

    if len(contents) > max_size:
        err_msg = "Refusing to parse, size of status exceeds character limit"
        LOGGER.warning(err_msg)
        raise XMLException(err_msg)

def check_string_size_for_events(contents):
    max_size = 5 * 1000 * 1000  # maximum size in bytes

    if len(contents.encode('utf-8')) > max_size:
        err_msg = "Refusing to parse, size of status exceeds size limit"
        LOGGER.warning(err_msg)
        raise XMLException(err_msg)