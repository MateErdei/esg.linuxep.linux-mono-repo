#!/usr/bin/env python3

import sys
import xml.dom.minidom


def main(argv):
    xmlfile = argv[1]
    repo = argv[2]
    branch = argv[3]

    dom = xml.dom.minidom.parse(open(xmlfile))
    open(xmlfile+".orig.xml", "wb").write(dom.toxml(encoding="utf-8"))

    buildAssetNodes = dom.getElementsByTagName("build-asset")
    for node in buildAssetNodes:
        if node.getAttribute("repo") == repo:
            developmentVersionNodes = node.getElementsByTagName("development-version")
            for n in developmentVersionNodes:
                n.setAttribute("branch", branch)

    output = dom.toxml(encoding="utf-8")
    open(xmlfile, "wb").write(output)
    dom.unlink()
    return 0


sys.exit(main(sys.argv))
