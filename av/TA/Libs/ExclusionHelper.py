#!/bin/env python

import os
import xml.dom.minidom


def __generate_root_exclusions():
    for t in os.listdir("/"):
        if os.path.isdir("/"+t):
            yield "/%s/" % t
        else:
            yield "/%s" % t


def __generate_exclusions_to_scan_tmp_test():
    for t in __generate_root_exclusions():
        if t == "/tmp_test/":
            continue
        yield t


def get_exclusions_to_scan_tmp_test():
    return list(__generate_exclusions_to_scan_tmp_test())


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


def replace_exclusions_in_policy_return_string(source, exclusions):
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
    for e in exclusions:
        fileset_element.appendChild(_filepath_node(e))

    out = dom.toprettyxml()
    dom.unlink()
    return out


def replace_exclusions_in_policy(source, destination, exclusions):
    open(destination, "w").write(replace_exclusions_in_policy_return_string(source, exclusions))


def Fill_In_On_Demand_Posix_Exclusions(source, destination):
    return replace_exclusions_in_policy(source, destination, __generate_exclusions_to_scan_tmp_test())


def Replace_Exclusions_For_Exclusion_Test(sourceFile):
    exclusions = []
    for excl in __generate_root_exclusions():
        if excl not in (
                "/directory_excluded/",
                "/file_excluded",
                "/file_excluded/",
                "/eicar.com"
        ):
            exclusions.append(excl)
    exclusions += [
                    "/directory_excluded/",
                    "/file_excluded",
                    "/eicar.com"
    ]
    return replace_exclusions_in_policy_return_string(sourceFile, exclusions)

def Get_Sav_Policy_With_No_Exclusions(sourceFile):
    return replace_exclusions_in_policy_return_string(sourceFile, [])
