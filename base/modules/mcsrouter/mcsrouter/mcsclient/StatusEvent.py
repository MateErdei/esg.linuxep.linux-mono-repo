#!/usr/bin/env python
# Copyright (C) 2017 Sophos Plc, Oxford, England.
# All rights reserved.


from __future__ import print_function, division, unicode_literals

import xml.dom.minidom

TEMPLATE_STATUS_EVENT = """<?xml version="1.0" encoding="utf-8" ?>
<ns:statuses schemaVersion="1.0" xmlns:ns="http://www.sophos.com/xml/mcs/statuses"></ns:statuses>"""


def textNode(doc, name, value):
    """
    textNode
    """
    element = doc.createElement(name)
    text = doc.createTextNode(value)
    element.appendChild(text)
    return element


class StatusEvent(object):
    """
    StatusEvent
    """
    def __init__(self):
        """
        __init__
        """
        self.__m_adapters = {}

    def add_adapter(self, app_id, ttl, creation_time, adapter_status_xml):
        """
        add_adapter
        """
        self.__m_adapters[app_id] = (ttl, creation_time, adapter_status_xml)

    def xml(self):
        """
        xml
        """
        doc = xml.dom.minidom.parseString(TEMPLATE_STATUS_EVENT)
        statuses = doc.getElementsByTagName("ns:statuses")[0]

        for (app_id, (ttl, creation_time, adapter_status_xml)
             ) in self.__m_adapters.iteritems():

            if isinstance(ttl, int):
                ttl = "PT%dS" % ttl
            status = doc.createElement("status")
            status.appendChild(textNode(doc, "appId", app_id))
            status.appendChild(textNode(doc, "creationTime", creation_time))
            status.appendChild(textNode(doc, "ttl", ttl))
            status.appendChild(textNode(doc, "body", adapter_status_xml))
            statuses.appendChild(status)

        output = doc.toxml(encoding="utf-8")
        doc.unlink()
        return output
