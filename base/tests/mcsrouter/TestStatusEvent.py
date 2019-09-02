#!/usr/bin/env python



import unittest
import sys

import logging
logger = logging.getLogger("TestStatusEvent")

import PathManager


import mcsrouter.mcsclient.status_event


class TestStatusEvent(unittest.TestCase):
    def testNoAdapters(self):
        statusEvent = mcsrouter.mcsclient.status_event.StatusEvent()
        x = statusEvent.xml()
        self.assertEqual(x, b"""<?xml version="1.0" encoding="utf-8"?><ns:statuses schemaVersion="1.0" xmlns:ns="http://www.sophos.com/xml/mcs/statuses"/>""")

    def testOneAdapter(self):
        statusEvent = mcsrouter.mcsclient.status_event.StatusEvent()
        statusEvent.add_adapter("AGENT",10000,"2020-04-01T10:08:41.1Z","""<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ns:computerStatus xmlns:ns="http://www.sophos.com/xml/mcs/computerstatus">
<meta protocolVersion="1.0" timestamp="2013-10-03T14:00:21.185617Z"/>
<commonComputerStatus>
<domainName></domainName>
<computerName>abelard</computerName>
<computerDescription></computerDescription>
<operatingSystem>linux</operatingSystem>
<lastLoggedOnUser>root@abelard</lastLoggedOnUser>
<ipv4>10.101.100.166</ipv4>
<ipv6></ipv6>
<fqdn>abelard</fqdn>
<processorArchitecture>x86_64</processorArchitecture>
</commonComputerStatus>
<posixPlatformDetails>
<platform>linux</platform>
<vendor>ubuntu</vendor>
<osMajorVersion>12</osMajorVersion>
<osMinorVersion>04</osMinorVersion>
<osMinimusVersion>3</osMinimusVersion>
<kernelVersion>3.5.0-40-generic</kernelVersion>
</posixPlatformDetails>
</ns:computerStatus>""")
        logger.debug(statusEvent.xml())
# except ImportError:
#     logger.error("Bad sys.path: %s",str(sys.path))
#     class TestStatusEvent(unittest.TestCase):
#         pass

if __name__ == '__main__':
    unittest.main()


