#!/usr/bin/env python3

import json
import os
import re
import sys
import xml.dom.minidom

import yaml
try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper


PARTS_RE = re.compile(r"esg-releasable-candidate/([^/.]+)\.([^/]+)/([^/]+)/([^/]+)/.*SDDS3-PACKAGE/")


def set_release_version(node, branch, build_id):
    node.setAttribute("branch", branch)
    node.setAttribute("build-id", build_id)
    return node


def create_new_release_version(dom, branch, build_id):
    node = dom.createElement("release-version")
    return set_release_version(node, branch, build_id)


def update_release_version(dom, key: str, artifact: str):
    mo = PARTS_RE.match(artifact)
    if not mo:
        print("Failed to match:", key, artifact)
        return
    project = mo.group(1)
    component = mo.group(2)
    branch = mo.group(3)
    build_id = mo.group(4)
    build_asset_nodes = dom.getElementsByTagName("build-asset")
    for node in build_asset_nodes:
        if node.getAttribute("project") != project:
            continue
        if node.getAttribute("repo") != component:
            continue
        print(key, project, component, branch, build_id)
        print("\t", node.getAttribute("repo"))
        new_node = create_new_release_version(dom, branch, build_id)
        for n in node.getElementsByTagName("release-version") + node.getElementsByTagName("development-version"):
            if new_node is not None:
                node.replaceChild(new_node, n)
                new_node = None
            else:
                node.removeChild(n)


def main(argv):
    if len(argv) != 3:
        print("Usage: python3 SupportFiles/update_dev_xml_from_pkg_latest.py def-sdds3/components/pkgs_latest.yaml build/dev.xml")
        return 1
    data = open(argv[1]).read()
    xmlfile = argv[2]

    dom = xml.dom.minidom.parse(open(xmlfile))
    open(xmlfile+".orig.xml", "wb").write(dom.toxml(encoding="utf-8"))

    data = yaml.load(data, Loader)
    # print(json.dumps(data, indent=2))
    for key, value in data.items():
        try:
            update_release_version(dom, key, value['prod-artifact'])
        except KeyError:
            print(key, json.dumps(value, indent=2))

    output = dom.toxml(encoding="utf-8")
    dom.unlink()

    outfilepath = xmlfile
    if os.environ.get("DEBUG", "0") == "1":
        outfilepath = xmlfile + ".new.xml"
    open(outfilepath, "wb").write(output)

    return 0


sys.exit(main(sys.argv))
