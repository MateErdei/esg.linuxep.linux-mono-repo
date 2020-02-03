#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.

import re
import sys
import xml.dom.minidom


def printPrintXML(data):
    dom = xml.dom.minidom.parseString(data)
    print(dom.toprettyxml(indent="  "))


def unescapeData(data):
    data = data.replace("\n", "")
    while "&amp;" in data:
        data = data.replace("&amp;", "&")

    data = data.replace("&lt;", "<")
    data = data.replace("&gt;", ">")
    data = data.replace("&quot;", '"')
    data = re.sub(r'(.)<\?xml version="1\.0" encoding="(utf|UTF)-8"(| standalone="yes")\?>', r"\1", data)

    try:
        printPrintXML(data)
        return 0
    except Exception as e:
        print(e, file=sys.stderr)
        print(data)
        return 1

def unescape(f):
    data = open(f).read()
    return unescapeData(data)


def main(argv):
    if len(argv) > 1:
        for f in argv[1:]:
            unescape(f)
    else:
        data = sys.stdin.read()
        return unescapeData(data)


if __name__ == '__main__':
    sys.exit(main(sys.argv))

