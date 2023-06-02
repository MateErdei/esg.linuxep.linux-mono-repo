#!/usr/bin/env python3

import json
import re
import sys

import yaml
try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper


PARTS_RE = re.compile(r"esg-releasable-candidate/([^/.]+)\.([^/]+)/([^/]+)/([^/]+)/.*SDDS3-PACKAGE/")


def print_release_version(key: str, artifact: str):
    mo = PARTS_RE.match(artifact)
    if not mo:
        print("Failed to match:", artifact)
        return

    project = mo.group(1)
    component = mo.group(2)
    branch = mo.group(3)
    build_id = mo.group(4)
    output = f"<release-version branch=\"{branch}\" build-id=\"{build_id}\"/>"
    print(key, project, component)
    print(output)


def main(argv):
    if len(argv) != 2:
        print("Usage: python3 SupportFiles/dump_suite_packages_as_release_version.py def-sdds3/components/pkg_latest.yaml")
        return 1
    data = open(argv[1]).read()
    data = yaml.load(data, Loader)
    # print(json.dumps(data, indent=2))
    for key, value in data.items():
        try:
            print_release_version(key, value['prod-artifact'])
        except KeyError:
            print(key, json.dumps(value, indent=2))
    return 0


sys.exit(main(sys.argv))