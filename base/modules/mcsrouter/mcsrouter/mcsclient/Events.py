#!/usr/bin/env python
"""
Events Module
"""

import xml.dom.minidom

import logging
LOGGER = logging.getLogger(__name__)

EVENTS_TEMPLATE = u"""<?xml version="1.0" encoding="UTF-8"?>
<ns:events schemaVersion="1.0" xmlns:ns="http://www.sophos.com/xml/mcs/events">
</ns:events>
"""
#~
#~ """<?xml version="1.0" encoding="UTF-8"?>
#~ <ns:events schemaVersion="1.0" xmlns:ns="http://www.sophos.com/xml/mcs/events">
#~ <event>
#~ <app_id>SAV</app_id>
#~ <creation_time>2013-10-04T09:24:47.806875Z</creation_time>
#~ <ttl>PT1209598S</ttl>
#~ <body>&lt;?xml version="1.0" encoding="UTF-8" standalone="yes"?&gt;
#~ &lt;notification xmlns="http://www.sophos.com/EE/Event" description="Virus/spyware 'EICAR-AV-Test' has been detected in &amp;quot;\\allegro.eng.sophos\peanut\eicar\eicar.com&amp;quot;.&amp;#xA;" type="sophos.mgt.msg.event.threat" timestamp="20131004 092447"&gt;&lt;user userId="TestUser" domain="NONWINXPSP3"/&gt;&lt;threat type="1" name="EICAR-AV-Test" scanType="200" status="200" id="2bf63a1291b939bdadbd53fa9ed048e2" idSource="NameFilenameFilepathCIMD5"&gt;&lt;item file="eicar.com" path="\\allegro.eng.sophos\peanut\eicar\"/&gt;&lt;action action="116"/&gt;&lt;/threat&gt;&lt;entity&gt;&lt;productId&gt;SAVEEXP&lt;/productId&gt;&lt;product-version&gt;10.3.3 VDL4.93G&lt;/product-version&gt;&lt;entityInfo&gt;SAVXP.10.3.3 VDL4.93G&lt;/entityInfo&gt;&lt;/entity&gt;&lt;/notification&gt;
#~ </body>
#~ <seq>0</seq>
#~ <id>20131004092447000d</id>
#~ </event>
#~ </ns:events>
#~ """


def text_node(doc, name, value):
    """
    text_node
    """
    element = doc.createElement(name)
    text = doc.createTextNode(value)
    element.appendChild(text)
    return element


class Event(object):
    """
    Event
    """

    def __init__(self, app_id, creation_time, ttl, body, seq, id):
        """
        __init__
        """
        self.m_app_id = app_id
        self.m_creation_time = creation_time
        self.m_ttl = ttl
        self.m_body = body
        self.m_seq = str(seq)
        self.m_id = id

    def add_xml(self, node, doc):
        """
        add_xml
        """
        event_node = doc.createElement(u"event")
        event_node.appendChild(text_node(doc, u"appId", self.m_app_id))
        event_node.appendChild(
            text_node(
                doc,
                u"creationTime",
                self.m_creation_time))
        event_node.appendChild(text_node(doc, u"ttl", self.m_ttl))
        event_node.appendChild(text_node(doc, u"body", self.m_body))
        event_node.appendChild(text_node(doc, u"seq", self.m_seq))
        event_node.appendChild(text_node(doc, u"id", self.m_id))

        node.appendChild(event_node)


class Events(object):
    """
    Events
    """

    def __init__(self):
        """
        __init__
        """
        self.__m_events = []
        self.__m_seq = 0

    def add_event(self, app_id, body, creation_time, ttl, id):
        """
        add_event
        """
        if isinstance(ttl, int):
            ttl = u"PT%dS" % ttl
        seq = self.__m_seq
        self.__m_seq += 1
        self.__m_events.append(
            Event(
                app_id,
                creation_time,
                ttl,
                body,
                seq,
                id))

    def xml(self):
        """
        xml
        """
        doc = xml.dom.minidom.parseString(EVENTS_TEMPLATE)
        events_node = doc.getElementsByTagName(u"ns:events")[0]
        for event in self.__m_events:
            LOGGER.info(
                "Sending event %s for %s adapter",
                event.m_id,
                event.m_app_id)
            event.add_xml(events_node, doc)

        output = doc.toxml(encoding="utf-8").decode("utf-8")
        doc.unlink()
        return output

    def reset(self):
        """
        reset
        """
        self.__m_events = []
        self.__m_seq = 0

    def has_events(self):
        """
        has_events
        """
        if self.__m_events:
            return True
        return False
