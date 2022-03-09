#!/usr/bin/env python3

from pathlib import Path
import os
import xmltodict
import sys
import json


def find_all_release_xmls(start_dir):
    release_package_names = ["release-package.xml", "release-package-linux.xml", "linux-release-package.xml",
                             "linux_package.xml", "sed-libs-linux-package.xml", "dev.xml", "sdds3.xml",
                             "capsule8-content-package.xml", "capsule8-sensor-package.xml", "package.xml"]
    release_file_locations = []
    for release_package_name in release_package_names:
        for filename in Path(start_dir).rglob(release_package_name):
            release_file_locations.append(filename)
    return release_file_locations


def process_release_files(release_files):
    all_dict = {}
    for release_file_path in release_files:
        with open(release_file_path, 'r') as release_file:
            content = release_file.read()
            as_dictionary = xmltodict.parse(content, dict_constructor=dict)
            name = as_dictionary['package']['@name']
            version = as_dictionary['package']['@version'].replace("-", ".")

            try:
                # Check the component has inputs
                if "package" not in as_dictionary or "inputs" not in as_dictionary['package'] or "build-asset" not in as_dictionary['package']['inputs']:
                    print(f"Skipping: {name} as it has no inputs")
                    continue
            except Exception as e:
                print(f"Falied to process: {name}")
                print(f"Dictionary: {as_dictionary}")
                print(f"Exception: {e}")

            all_dict[name] = {}
            this_entry = all_dict[name]
            this_entry["version"] = version
            inputs = []

            # Ensure that this is a list in all circumstances (it defaults to a single dictionary if only one input)
            pkg_inputs_list = as_dictionary["package"]["inputs"]["build-asset"]
            if not isinstance(pkg_inputs_list, list):
                pkg_inputs_list = [pkg_inputs_list]

            for input_pkg in pkg_inputs_list:
                if 'release-version' not in input_pkg:
                    print(f"Skipping input: {input_dict['name']} in: {release_file_path}, as there is no release-version")
                    continue

                input_dict = {"name": input_pkg["@repo"], "build_id": input_pkg["release-version"]["@build-id"]}

                print(input_dict["name"])
                print(input_dict["build_id"])
                inputs.append(input_dict)

            this_entry["inputs"] = inputs
    return all_dict


def main(argv):
    to_search = os.path.dirname(__file__) + "/.."
    if len(argv) > 1 and os.path.isdir(to_search):
        to_search = argv[1]

    release_files = find_all_release_xmls(to_search)
    as_dict = process_release_files(release_files)
    as_json = json.dumps(as_dict, sort_keys=True, indent=4)
    print(as_json)

    with open("releaseInputs.json", 'w') as f:
        f.write(as_json)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
