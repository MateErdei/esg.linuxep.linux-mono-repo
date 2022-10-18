#!/usr/bin/env python3

import sys
import xml.dom.minidom


def update(dom, repo, branch):
    buildAssetNodes = dom.getElementsByTagName("build-asset")
    for node in buildAssetNodes:
        if node.getAttribute("repo") == repo:
            developmentVersionNodes = node.getElementsByTagName("development-version")
            for n in developmentVersionNodes:
                n.setAttribute("branch", branch)


def pairs(lst):
    for i in range(0, len(lst), 2):
        yield lst[i:i+2]


def main(argv):
    xmlfile = argv[1]
    repo = argv[2]
    branch = argv[3]

    dom = xml.dom.minidom.parse(open(xmlfile))
    open(xmlfile+".orig.xml", "wb").write(dom.toxml(encoding="utf-8"))

    fields = argv[2:]
    for (repo, branch) in pairs(fields):
        update(dom, repo, branch)

    output = dom.toxml(encoding="utf-8")
    open(xmlfile, "wb").write(output)
    dom.unlink()
    return 0


sys.exit(main(sys.argv))
