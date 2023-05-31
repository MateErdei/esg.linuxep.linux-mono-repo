#!/usr/bin/env python3

import json
import os
import sys
import urllib.request
import xml.dom.minidom

import yaml
try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper


def get_version(repository_path):
    """
    Convert
    esg-releasable-candidate/linuxep.sspl-plugin-anti-virus/release--2023.2-DTP1/20230531081647-0913f58259eed5aa7ad386a9508739b0d9f8fc16-DAmXzD/sspl-plugin-anti-virus/SDDS3-PACKAGE/
    to
    https://artifactory.sophos-ops.com/artifactory/esg-releasable-candidate/linuxep.sspl-plugin-anti-virus/release--2023.2-DTP1/20230531081647-0913f58259eed5aa7ad386a9508739b0d9f8fc16-DAmXzD/sspl-plugin-anti-virus/SDDS3-PACKAGE/SDDS-Import.xml
    :param repository_path:
    :return:
    """
    url = "https://artifactory.sophos-ops.com/artifactory/" + repository_path + "SDDS-Import.xml"
    response = urllib.request.urlopen(url)
    xml_data = response.read()
    # print(xml_data)
    dom = xml.dom.minidom.parseString(xml_data)
    componentdata = dom.getElementsByTagName("ComponentData")
    for n in componentdata:
        versions = n.getElementsByTagName("Version")
        for v in versions:
            text = ""
            for node in v.childNodes:
                text += node.data
            return text


def main(argv):
    if len(argv) != 2:
        print("Usage: python3 SupportFiles/extractVersions.py def-sdds3/components/suite_packages.yaml")
        return 1
    data = open(argv[1]).read()
    data = yaml.load(data, Loader)
    # print(json.dumps(data, indent=2))
    for key, value in data.items():
        version = get_version(value['prod-artifact'])
        component = key.split(".")[0]
        build_id = value['prod-artifact'].split("/")[3]
        print(component, version, build_id)
    return 0


sys.exit(main(sys.argv))
