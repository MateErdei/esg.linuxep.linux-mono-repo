#!/usr/bin/env python3

import os
import sys
import json


def get_component_using_input_build_id(root_dictionary, input_name, input_branch, input_build_id):
    components_using_input_build_id = set()
    for component_name, component_info in root_dictionary.items():
        for i in component_info["inputs"]:
            if i["name"] == input_name and i["branch"] == input_branch and i["build_id"] == input_build_id:
                components_using_input_build_id.add(component_name)
    return components_using_input_build_id

# this function should be extended when/if more filters are required
def filter_inputs(info_dict):
    for artifact_dict in info_dict["artifacts"]:
        if "susi_input" in artifact_dict["@dest-dir"]:
            # if artifact is a susi input we don't care about it as these inputs are dictated by core
            return False
    return True

def verify_json(json_string):
    root_dict = json.loads(json_string)
    all_inputs = {}
    
    inputs_with_accepted_inconsistencies = ["esg", "everest-base", "sau"]
    
    for component_name, info_dict in root_dict.items():
        for i in filter(filter_inputs, info_dict["inputs"]):
            if i["name"] not in all_inputs:
                all_inputs[i["name"]] = set()
            all_inputs[i["name"]].add(f"{i['branch']}*{i['build_id']}")
    found_inconsistencies = False
    ignored_inconsistencies = False
    for input_name, input_branch_and_build_ids in all_inputs.items():
        if (len(input_branch_and_build_ids)) > 1:
            if input_name in inputs_with_accepted_inconsistencies:
                ignored_inconsistencies = True
                print("WARNING:")
            else:
                found_inconsistencies = True
                print("ERROR:")

            for branch_and_build_id in input_branch_and_build_ids:
                input_branch = branch_and_build_id.split('*')[0]
                build_id = branch_and_build_id.split('*')[1]
                for c in get_component_using_input_build_id(root_dict, input_name, input_branch, build_id):
                    print(f"Inconsistent branch and build_ids, input: {input_name}, branch: {input_branch}, "
                          f"build_id: {build_id}, component using it: {c}")

    if found_inconsistencies:
        print("FAILED: Inputs are inconsistent, refer to logged errors")
        exit(1)

    if ignored_inconsistencies:
        print("UNSTABLE: Inconsistent inputs have been accepted, refer to logged warnings")
        exit(2)


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
