#!/usr/bin/env python

import xml.dom.minidom

import logging
logger = logging.getLogger(__name__)

EVENTS_TEMPLATE=u"""<?xml version="1.0" encoding="UTF-8"?>
<ns:events schemaVersion="1.0" xmlns:ns="http://www.sophos.com/xml/mcs/events">
</ns:events>
"""
#~
#~ """<?xml version="1.0" encoding="UTF-8"?>
#~ <ns:events schemaVersion="1.0" xmlns:ns="http://www.sophos.com/xml/mcs/events">
#~ <event>
#~ <appId>SAV</appId>
#~ <creationTime>2013-10-04T09:24:47.806875Z</creationTime>
#~ <ttl>PT1209598S</ttl>
#~ <body>&lt;?xml version="1.0" encoding="UTF-8" standalone="yes"?&gt;
#~ &lt;notification xmlns="http://www.sophos.com/EE/Event" description="Virus/spyware 'EICAR-AV-Test' has been detected in &amp;quot;\\allegro.eng.sophos\peanut\eicar\eicar.com&amp;quot;.&amp;#xA;" type="sophos.mgt.msg.event.threat" timestamp="20131004 092447"&gt;&lt;user userId="TestUser" domain="NONWINXPSP3"/&gt;&lt;threat type="1" name="EICAR-AV-Test" scanType="200" status="200" id="2bf63a1291b939bdadbd53fa9ed048e2" idSource="NameFilenameFilepathCIMD5"&gt;&lt;item file="eicar.com" path="\\allegro.eng.sophos\peanut\eicar\"/&gt;&lt;action action="116"/&gt;&lt;/threat&gt;&lt;entity&gt;&lt;productId&gt;SAVEEXP&lt;/productId&gt;&lt;product-version&gt;10.3.3 VDL4.93G&lt;/product-version&gt;&lt;entityInfo&gt;SAVXP.10.3.3 VDL4.93G&lt;/entityInfo&gt;&lt;/entity&gt;&lt;/notification&gt;
#~ </body>
#~ <seq>0</seq>
#~ <id>20131004092447000d</id>
#~ </event>
#~ </ns:events>
#~ """

def textNode(doc, name, value):
    element = doc.createElement(name)
    text = doc.createTextNode(value)
    element.appendChild(text)
    return element

class Event(object):
    def __init__(self, appId, creationTime, ttl, body, seq, id):
        self.m_appId = appId
        self.m_creationTime = creationTime
        self.m_ttl = ttl
        self.m_body = body
        self.m_seq = str(seq)
        self.m_id = id

    def addXML(self, node, doc):
        eventNode = doc.createElement(u"event")
        eventNode.appendChild(textNode(doc, u"appId",self.m_appId))
        eventNode.appendChild(textNode(doc, u"creationTime",self.m_creationTime))
        eventNode.appendChild(textNode(doc, u"ttl",self.m_ttl))
        eventNode.appendChild(textNode(doc, u"body",self.m_body))
        eventNode.appendChild(textNode(doc, u"seq",self.m_seq))
        eventNode.appendChild(textNode(doc, u"id",self.m_id))

        node.appendChild(eventNode)

class Events(object):
    def __init__(self):
        self.__m_events = []
        self.__m_seq = 0

    def addEvent(self, appId, body, creationTime, ttl, id):
        if isinstance(ttl,int):
            ttl = u"PT%dS"%ttl
        seq = self.__m_seq
        self.__m_seq += 1
        self.__m_events.append(Event(appId, creationTime, ttl, body, seq, id))

    def xml(self):
        doc = xml.dom.minidom.parseString(EVENTS_TEMPLATE)
        eventsNode = doc.getElementsByTagName(u"ns:events")[0]
        for event in self.__m_events:
            logger.info("Sending event %s for %s adapter", event.m_id, event.m_appId)
            event.addXML(eventsNode, doc)

        output = doc.toxml(encoding="utf-8").decode("utf-8")
        doc.unlink()
        return output

    def reset(self):
        self.__m_events = []
        self.__m_seq = 0

    def hasEvents(self):
        return len(self.__m_events) > 0



