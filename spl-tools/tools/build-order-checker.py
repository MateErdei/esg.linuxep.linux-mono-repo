#!/usr/bin/env python3

import sys
import json
import requests
import urllib3
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)


def get_components_using_component_as_input(input_component, known_components):
    components_using_component = set()
    for component_name, component_info in known_components.items():
        for i in component_info["inputs"]:
            if i["name"] == input_component:
                components_using_component.add(component_name)
    return list(components_using_component)


def get_all(components, known_components):
    all = []
    if isinstance(components, (list, set)):
        for c in components:
            for a in get_components_using_component_as_input(c, known_components):
                all.append(a)
    else:
        return get_components_using_component_as_input(components, known_components)
    return list(set(all))


def print_build_order(component_csv, known_components):
    input_component_list = component_csv.split(',')

    level = 0
    levels = {level: input_component_list}
    next_level_of_components = None
    while next_level_of_components is None or len(next_level_of_components) > 0:
        next_level_of_components = get_all(levels[level], known_components)
        level += 1
        levels[level] = next_level_of_components.copy()

    seen = []
    i = len(levels) - 1
    while i > 0:
        to_remove = []
        for component_in_level in levels[i]:
            if component_in_level in seen:
                to_remove.append(component_in_level)
            else:
                seen.append(component_in_level)
        for r in to_remove:
            levels[i].remove(r)
        i -= 1

    print("Build order:")
    for level_index, components_in_level in levels.items():
        if len(components_in_level) > 0:
            print("{}:{}".format(level_index, components_in_level))
    return


def list_all(json_string):
    root_dict = json.loads(json_string)
    print("\nComponents:")
    for a in root_dict:
        print(a)


def print_help():
    print("List: python3 build-order-checker.py list")
    print("Example1, what do I need to build if I upgrade openssl: python3 build-order-checker.py openssllinux11")
    print("Example2, what do I need to build if I upgrade openssl and expat: python3 build-order-checker.py openssllinux11,expatlinux11")
    print("The order in which to build components is then printed.")


def main(argv):

    cmd = "help"
    url_to_json = "https://sspljenkins.eng.sophos/job/Generate-Release-Package-Input-Data/HTML_20Report/releaseInputs.json"
    if len(argv) == 2:
        cmd = argv[1]
    else:
        print("Pass in either 'list' or the component you want to know the build order of.")
        exit(1)

    r = requests.get(url=url_to_json, verify=False)
    json_string = r.text

    if json_string == "":
        print("ERROR: No JSON input specified.")
        exit(1)

    if cmd == "list":
        list_all(json_string)

    if cmd in ["help", "--help", " -h"]:
        print_help()

    known_components = json.loads(json_string)
    print_build_order(cmd, known_components)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
