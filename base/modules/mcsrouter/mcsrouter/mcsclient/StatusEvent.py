#!/usr/bin/env python
# Copyright (C) 2017 Sophos Plc, Oxford, England.
# All rights reserved.


from __future__ import print_function,division,unicode_literals

import xml.dom.minidom

TEMPLATE_STATUS_EVENT="""<?xml version="1.0" encoding="utf-8" ?>
<ns:statuses schemaVersion="1.0" xmlns:ns="http://www.sophos.com/xml/mcs/statuses"></ns:statuses>"""

def textNode(doc, name, value):
    element = doc.createElement(name)
    text = doc.createTextNode(value)
    element.appendChild(text)
    return element

class StatusEvent(object):
    def __init__(self):
        self.__m_adapters = {}

    def addAdapter(self, appID, ttl, creationTime, adapterStatusXML):
        """
        """
        self.__m_adapters[appID] = (ttl,creationTime,adapterStatusXML)

    def xml(self):
        doc = xml.dom.minidom.parseString(TEMPLATE_STATUS_EVENT)
        statuses = doc.getElementsByTagName("ns:statuses")[0]

        for (appID,(ttl,creationTime,adapterStatusXML)) in self.__m_adapters.iteritems():

            if isinstance(ttl,int):
                ttl = "PT%dS"%ttl
            status = doc.createElement("status")
            status.appendChild(textNode(doc,"appId",appID))
            status.appendChild(textNode(doc,"creationTime",creationTime))
            status.appendChild(textNode(doc,"ttl",ttl))
            status.appendChild(textNode(doc,"body",adapterStatusXML))
            statuses.appendChild(status)

        output = doc.toxml(encoding="utf-8")
        doc.unlink()
        return output
