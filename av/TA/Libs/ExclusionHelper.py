#!/bin/env python

import os
import xml.dom.minidom


def __generate_root_exclusions():
    for t in os.listdir("/"):
        if os.path.isdir("/"+t):
            yield "/%s/" % t
        yield "/%s" % t


def __generate_exclusions_to_scan_tmp():
    for t in __generate_root_exclusions():
        if t == "/tmp/":
            continue
        yield t


def get_exclusions_to_scan_tmp():
    return list(__generate_exclusions_to_scan_tmp())


def Get_Root_Exclusions_for_avscanner_except_proc():
    out = []
    for t in __generate_root_exclusions():
        if t == "/proc/":
            continue
        out.append(t)
    return " ".join(out)


def __remove_blanks(node):
    for x in node.childNodes:
        if x.nodeType == xml.dom.Node.TEXT_NODE:
            if x.nodeValue:
                x.nodeValue = x.nodeValue.strip()
        elif x.nodeType == xml.dom.Node.ELEMENT_NODE:
            __remove_blanks(x)

def Fill_In_On_Demand_Posix_Exclusions(source, destination):
    dom = xml.dom.minidom.parse(source)
    __remove_blanks(dom)

    def _filepath_node(text):
        n = dom.createElement('filePath')
        t = dom.createTextNode(text)
        n.appendChild(t)
        return n

    ondemand_element = dom.getElementsByTagName("onDemandScan")[0]
    posix_exclusions_element = ondemand_element.getElementsByTagName('posixExclusions')[0]
    fileset_element = posix_exclusions_element.getElementsByTagName('filePathSet')[0]
    # remove children
    while fileset_element.hasChildNodes():
        fileset_element.removeChild(fileset_element.lastChild)
    for e in __generate_exclusions_to_scan_tmp():
        fileset_element.appendChild(_filepath_node(e))

    open(destination, "w").write(dom.toprettyxml())
