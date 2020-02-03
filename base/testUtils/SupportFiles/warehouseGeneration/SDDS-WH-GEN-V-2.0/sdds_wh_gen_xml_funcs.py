#!/usr/bin/env python

__author__ = "John Moffa (john.moffa@sophos.com)"
__version__ = "$Revision: 1.0 $"
__date__ = "$Date: 2008/03/19 09:00:00 $"
__copyright__ = "What ever I finds, I keeps"
__license__ = "Python"

import sys
import os
from xml.dom import minidom
from xml.dom.minidom import Document

class XmlProcessing(object):

    def __init__(self, nodeType = None):

        # creates the minidom document
        self.__m_xmlDocument = Document()

        self.__m_customNodes = 0

        # specify what kind of node list is to be used
        if not nodeType:
            self.childNodes = {"node1":None,"node2":None,"node3":None,"node4":None,
                               "node5":None,"node6":None,"node7":None,"node8":None,
                               "node9":None,"node10":None,"node11":None,"node12":None,
                               "node13":None,"node14":None,"node15":None,"node16":None,
                               "node17":None,"node18":None,"node19":None,"node20":None,
                               "node21":None,"node22":None,"node23":None,"node24":None}
        else:
            self.childNodes = {}
            self.__m_customNodes = 1

    # --------------------------------------------------
    #
    # Saves the XML document to disk using the minidom
    # document as a source - should allow tidying up
    # of XML tags but causes SDDM retailer issues
    #
    # --------------------------------------------------
    def svXMLDoc(self, path):

        try:
            #print "saving xml to %s" % path
            xmlFile = open(path, "wb")
        except IOError:
            print "XML file failed to save correctly."
            return None
        else:
            self.__m_xmlDocument.toprettyxml(indent="  ")
            self.__m_xmlDocument.normalize()
            #self.__m_xmlDocument.writexml(xmlFile, "    ", "", "\n", "UTF-8")
            self.__m_xmlDocument.writexml(xmlFile, "", "", "", "UTF-8")
            xmlFile.close()
            return xmlFile

    def __setAttribute(self, node, key, value):
        assert node is not None
        assert key is not None
        assert value is not None
        node.setAttribute(key, value)

    def __setAttributes(self, node, attributes):
        assert node is not None
        for (k,v) in attributes.iteritems():

            assert k is not None
            #~ assert v is not None,"Value for %s is None for node %s"%(k,node.nodeName)
            if v is None:
                print("Excluding attribute %s for %s"%(k,node.nodeName))
                continue

            assert(isinstance(k,basestring))
            assert(isinstance(v,basestring))
            node.setAttribute(k,v)

    # --------------------------------------------------
    #
    # Create top level XML node
    #
    # --------------------------------------------------
    def crParElem(self, elementName = "", label = "", value = "",
                  label2 = "", value2 = ""):
        assert elementName is not None
        self.__m_parent = self.__m_xmlDocument.createElement(elementName)
        if label:
            self.__setAttribute(self.__m_parent,label, value)
        if label2:
            self.__setAttribute(self.__m_parent,label2, value2)
        self.__m_xmlDocument.appendChild(self.__m_parent)

    # --------------------------------------------------
    #
    # Create a child attribute of which the parent is
    # specified as tag string corresponding to
    # dictionary entry (XML node list)
    #
    # --------------------------------------------------
    def crChldElem(self, parent, child, attributeName, key = None, value = []):
        # check if we're using custom nodes
        if self.__m_customNodes == 1:

            # create new node in libary under specific name
            self.childNodes[child] = None

        assert attributeName is not None
        assert child is not None

        thing = self.__m_xmlDocument.createElement(attributeName)
        self.childNodes[child] = thing

        if isinstance(key,list):
            self.__setAttributes(self.childNodes[child], dict(zip(key,value)))
        elif isinstance(key,dict):
            self.__setAttributes(self.childNodes[child], key)

        if parent == "parent":
            self.__m_parent.appendChild(self.childNodes[child])
        else:
            self.childNodes[parent].appendChild(self.childNodes[child])

    def chlTxtNode(self, parent, textContents = ""):
        assert textContents is not None
        text = self.__m_xmlDocument.createTextNode(textContents)
        self.childNodes[parent].appendChild(text)
