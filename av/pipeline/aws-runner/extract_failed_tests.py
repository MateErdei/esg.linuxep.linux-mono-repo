#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import sys
import time
import xml.etree.ElementTree as ET

def process_test(basename, test, chain):
    for status in test.findall("status"):
        result = status.attrib['status']
    if result == "PASS":
        return
    print(result+":", basename + chain + "/" + test.attrib['name'])


def process_suite(basename, node, chain=""):
    name = node.attrib['name']
    chain += "/"+name

    for suite in node.findall("suite"):
        process_suite(basename, suite, chain)
    for test in node.findall("test"):
        process_test(basename, test, chain)

def process_xml(path):
    basename = os.path.basename(path)
    if basename.endswith(".xml"):
        basename = basename[:-4]

    start = time.time()
    tree = ET.parse(path)
    root = tree.getroot()
    end = time.time()
    # print("Parsing {} took {} seconds".format(basename, end-start))

    for suite in root.findall("suite"):
        for s in suite.findall("suite"):
            process_suite(basename, s)

    # for element in root.iter():
    #     print("E", element)


def main(argv):
    for path in argv[1:]:
        process_xml(path)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
