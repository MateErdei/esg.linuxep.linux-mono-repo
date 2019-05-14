#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals
import unittest
import sys

import PathManager
import mcsrouter.utils.xml_helper

class TestXmlHelder(unittest.TestCase):
    def testParseString(self):
        TEST_SRC="<a>FOO</a>"
        doc = mcsrouter.utils.xml_helper.parseString(TEST_SRC)
        a = doc.getElementsByTagName("a")
        self.assertEqual(len(a), 1)

        text = mcsrouter.utils.xml_helper.get_text_from_element(a[0])
        self.assertEqual(text, "FOO")

        doc.unlink()

    def testEntitesAreIgnored(self):
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
            pass


if __name__ == '__main__':
    unittest.main()