#!/usr/bin/env python3

from pathlib import Path
import os
import xmltodict
import sys
import json


# a > b returns 1, a < b returns -1, a = b returns 0
def compare_version(a, b):
    delim = "-"
    a_split = a.split(delim)
    b_split = b.split(delim)

    # cannot compare
    if len(a) != len(b):
        return None

    index = 0
    while index < len(a):
        if a_split[index] == b_split[index]:
            index += 1
            continue
        if int(a_split[index]) > int(b_split[index]):
            return 1
        else:
            return -1
    return 0


def get_component_using_input_version(root_dictionary, input_name, input_version):
    components_using_input_version = set()
    #print(root_dictionary)
    for component_name, component_info in root_dictionary.items():
        for i in component_info["inputs"]:
            if i["name"] == input_name and i["version"] == input_version:
                components_using_input_version.add(component_name)
    #print(components_using_input_version)
    return components_using_input_version


def verify_json(json_string):
    root_dict = json.loads(json_string)
    all_inputs = {}
    for component_name, info_dict in root_dict.items():
        for i in info_dict["inputs"]:
            if i["name"] not in all_inputs:
                all_inputs[i["name"]] = set()
            all_inputs[i["name"]].add(i["version"])
    found_inconsistencies = False
    #print(all_inputs)
    for input_name, input_versions in all_inputs.items():
        if (len(input_versions)) > 1:
            found_inconsistencies = True
            print("ERROR:")
            for version in input_versions:
                for c in get_component_using_input_version(root_dict, input_name, version):
                    print("Inconsistent versions, input: {}, version: {}, component using it: {}".format(input_name, version, c))

    if found_inconsistencies:
        print("FAILED: Inputs are inconsistent, refer to logged errors")
        exit(1)


def main(argv):

    json_string = ""

    # URL at time of writing: "https://10.55.36.24/job/Generate-Release-Package-Input-Data/HTML_20Report/releaseInputs.json"
    url_to_json = None
    if len(argv) > 1:
        url_to_json = argv[1]

    # If no url specified then try reading local file as this may be running manually.
    if url_to_json is None:
        json_file_source = "releaseInputs.json"
        if os.path.isfile(json_file_source):
            with open(json_file_source, 'r') as f:
                json_string = f.read()
    else:
        import requests
        r = requests.get(url=url_to_json, verify=False)
        json_string = r.text

    if json_string == "":
        print("ERROR: No JSON input specified.")
        exit(1)

    verify_json(json_string)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
