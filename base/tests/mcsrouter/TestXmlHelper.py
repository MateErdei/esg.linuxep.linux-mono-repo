#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals
import unittest

import PathManager

import mcsrouter.mcsclient.status_event
import mcsrouter.mcsclient.events
import mcsrouter.utils.xml_helper

class TestXmlHelper(unittest.TestCase):
    def testParseString(self):
        TEST_SRC="<a>FOO</a>"
        doc = mcsrouter.utils.xml_helper.parseString(TEST_SRC)
        a = doc.getElementsByTagName("a")
        self.assertEqual(len(a), 1)

        text = mcsrouter.utils.xml_helper.get_text_from_element(a[0])
        self.assertEqual(text, "FOO")

        doc.unlink()

    def testEntitiesAreIgnored(self):
        TEST_DOC="""<?xml version="1.0"?>
        <!DOCTYPE lolz [
         <!ENTITY lol "lol">
         <!ELEMENT lolz (#PCDATA)>
         <!ENTITY lol1 "&lol;&lol;&lol;&lol;&lol;&lol;&lol;&lol;&lol;&lol;">
         <!ENTITY lol2 "&lol1;&lol1;&lol1;&lol1;&lol1;&lol1;&lol1;&lol1;&lol1;&lol1;">
         <!ENTITY lol3 "&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;">
         <!ENTITY lol4 "&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;">
         <!ENTITY lol5 "&lol4;&lol4;&lol4;&lol4;&lol4;&lol4;&lol4;&lol4;&lol4;&lol4;">
         <!ENTITY lol6 "&lol5;&lol5;&lol5;&lol5;&lol5;&lol5;&lol5;&lol5;&lol5;&lol5;">
         <!ENTITY lol7 "&lol6;&lol6;&lol6;&lol6;&lol6;&lol6;&lol6;&lol6;&lol6;&lol6;">
         <!ENTITY lol8 "&lol7;&lol7;&lol7;&lol7;&lol7;&lol7;&lol7;&lol7;&lol7;&lol7;">
         <!ENTITY lol9 "&lol8;&lol8;&lol8;&lol8;&lol8;&lol8;&lol8;&lol8;&lol8;&lol8;">
        ]>
        <lolz>&lol9;</lolz>"""

        try:
            doc = mcsrouter.utils.xml_helper.parseString(TEST_DOC)
            doc.unlink()
            self.fail("Able to parse entities")
        except Exception as ex:
            assert(ex.message=="Refusing to parse Entity Declaration: lol")

    def testMissingClosingTagXMLThrows(self):
        TEST_DOC="""<?xml version="1.0"?>
        <ns:computerStatus xmlns:ns="http://www.sophos.com/xml/mcs/computerstatus">
        <stuff></stuff>
        """

        try:
            doc = mcsrouter.utils.xml_helper.parseString(TEST_DOC)
            doc.unlink()
            self.fail("should not be able to parse")
        except Exception as ex:
            assert(ex.message=="no element found: line 4, column 8")


    def testBrokenXMLThrows(self):
        TEST_DOC="""<?xml version="1.0"?>
        <ns:computerStatus xmlns:ns="http://www.sophos.com/xml/mcs/computerstatus">
        <stuff></ns:computerStatus></stuff>
        """

        try:
            doc = mcsrouter.utils.xml_helper.parseString(TEST_DOC)
            doc.unlink()
            self.fail("should not be able to parse")
        except Exception as ex:
            assert(ex.message=="mismatched tag: line 3, column 17")


    def testXMLWithXhtmlScriptTagThrows(self):
        TEST_DOC="""<xhtml:script xmlns:xhtml="http://www.sophos.com/xml/mcs/computerstatus"
        src="file.js"
        type="application/javascript"/>"""

        try:
            doc = mcsrouter.utils.xml_helper.parseStringAndRejectScriptElements(TEST_DOC)
            doc.unlink()
            self.fail("should not be able to parse")
        except Exception as ex:
            assert(ex.message=="Refusing to parse Script Element")


    def testXMLWithScriptTagThrows(self):
        TEST_DOC="""<script xmlns="http://www.sophos.com/xml/mcs/computerstatus" src="file.js"></script>"""

        try:
            doc = mcsrouter.utils.xml_helper.parseStringAndRejectScriptElements(TEST_DOC)
            doc.unlink()
            self.fail("should not be able to parse")
        except Exception as ex:
            assert(ex.message=="Refusing to parse Script Element")


    def testValidStatusXmlDoesntThrow(self):
        try:
            status = mcsrouter.mcsclient.status_event.StatusEvent()
            status.xml()
        except Exception as ex:
            self.fail(ex)


    def testValidEventXmlDoesntThrow(self):
        try:
            event = mcsrouter.mcsclient.events.Events()
            event.xml()
        except Exception as ex:
            self.fail(ex)


if __name__ == '__main__':
    unittest.main()
