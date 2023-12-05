#!/usr/bin/env python3

# This is used to print out the version numbers of a release build so that we can easily update sprint wiki pages.

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


def get_version(repository_path: str):
    """
    Convert
    esg-releasable-candidate/linuxep.sspl-plugin-anti-virus/release--2023.2-DTP1/20230531081647-0913f58259eed5aa7ad386a9508739b0d9f8fc16-DAmXzD/sspl-plugin-anti-virus/SDDS3-PACKAGE/
    to
    https://artifactory.sophos-ops.com/artifactory/esg-releasable-candidate/linuxep.sspl-plugin-anti-virus/release--2023.2-DTP1/20230531081647-0913f58259eed5aa7ad386a9508739b0d9f8fc16-DAmXzD/sspl-plugin-anti-virus/SDDS3-PACKAGE/SDDS-Import.xml
    :param repository_path:
    :return:
    """
    if not repository_path.endswith("/"):
        repository_path = repository_path + "/"

    url = "https://artifactory.sophos-ops.com/artifactory/" + repository_path + "SDDS-Import.xml"
    # print(f"GET: {url}")
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


def convert_to_dir_name(component: str):
    if component == "event-journaler":  # Name in suite
        return "eventjournaler"  # Actual directory name in mono repo
    if component == "liveresponse":
        return "liveterminal"
    if component == "responseactions":
        return "response_actions"
    # Most already match
    return component


def main(argv):
    if len(argv) != 4:
        print("Usage: python3 SupportFiles/extractVersions.py def-sdds3/components/pkgs_latest.yaml '<branch>' '<linux mono repo build ID>'")
        print("Example: python3 SupportFiles/extractVersions.py def-sdds3/components/pkgs_latest.yaml 'release--2023-49' '20231205090922-95d58331554228e82207ea98a25619e8bb9dc0ca-MsBIlm'")
        return 1

    path = argv[1]
    branch = argv[2]
    mono_repo_build_id = argv[3]

    if not os.path.exists(path):
        print(f"{path} does not exist")
        return 1

    data = open(path).read()
    data = yaml.load(data, Loader)
    # print(json.dumps(data, indent=2))
    for key, value in data.items():
        arch = key.split(".")[2]
        component_name_in_suite = key.split(".")[0]
        component = convert_to_dir_name(component_name_in_suite)
        if 'prod-artifact' in value:
            version = get_version(value['prod-artifact'])
            build_id = value['prod-artifact'].split("/")[3]
        else:
            build_id = mono_repo_build_id
            version = get_version(
                f"esg-releasable-candidate/linuxep.linux-mono-repo/{branch}/{build_id}/{component}/linux_x64_rel/installer")
        print(arch, component, version, build_id)
    return 0


sys.exit(main(sys.argv))
