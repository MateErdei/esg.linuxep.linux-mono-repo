#!/usr/bin/env python3

import os
import sys
import json


def get_component_using_input_build_id(root_dictionary, input_name, input_build_id):
    components_using_input_build_id = set()
    for component_name, component_info in root_dictionary.items():
        for i in component_info["inputs"]:
            if i["name"] == input_name and i["build_id"] == input_build_id:
                components_using_input_build_id.add(component_name)
    return components_using_input_build_id


def verify_json(json_string):
    root_dict = json.loads(json_string)
    all_inputs = {}
    for component_name, info_dict in root_dict.items():
        for i in info_dict["inputs"]:
            if i["name"] not in all_inputs:
                all_inputs[i["name"]] = set()
            all_inputs[i["name"]].add(i["build_id"])
    found_inconsistencies = False
    for input_name, input_build_ids in all_inputs.items():
        if (len(input_build_ids)) > 1:
            found_inconsistencies = True
            print("ERROR:")
            for build_id in input_build_ids:
                for c in get_component_using_input_build_id(root_dict, input_name, build_id):
                    print(f"Inconsistent build_ids, input: {input_name}, build_id: {build_id}, component using it: {c}")

    if found_inconsistencies:
        print("FAILED: Inputs are inconsistent, refer to logged errors")
        exit(1)


def main(argv):
    json_string = ""

    # URL at time of writing: "https://sspljenkins.eng.sophos/job/Generate-Release-Package-Input-Data/HTML_20Report/releaseInputs.json"
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
