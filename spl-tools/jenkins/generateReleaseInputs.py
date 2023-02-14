#!/usr/bin/env python3

from pathlib import Path
import os
import xmltodict
import sys
import json

repoNameToPackageName = {
    "liveterminal": "liveterminal_linux",
    "capsule8-sensor": "runtimedetections",
    "sau": "sau_xg",
    "sed": "sed",
    "everest-base": "sspl-base",
    "sspl-plugin-edr-componentsuite": "sspl-componentsuite",
    "sspl-plugin-event-journaler": "sspl-event-journaler-plugin",
    "sspl-flags": "sspl-flags",
    "sspl-plugin-mdr-component": "sspl-mdr-control-plugin",
    "sspl-plugin-anti-virus": "sspl-plugin-anti-virus",
    "sspl-plugin-edr-component": "sspl-plugin-edr-component",
    "thininstaller": "sspl-thininstaller",
    "sspl-warehouse": "sspl-warehouse",
    "versig": "versig",
    "livequery": "xdrsharedcomponents",
    "boost": "boostlinux11",
    "capnproto": "capnprotolinux11",
    "cmake": "cmakelinux",
    "curl": "curllinux11",
    "libexpat": "expatlinux11",
    "flatbuffers": "flatbuffers",
    "gflags": "gflagslinux11",
    "glog": "gloglinux11",
    "googletest": "googletest",
    "grpc": "grpc",
    "jsoncpp": "jsoncpplinux11",
    "libffi": "libffi",
    "libxcrypt": "libxcrypt",
    "log4cplus": "log4cpluslinux11",
    "minizip": "miniziplinux11",
    "openssl": "openssllinux11",
    "protobuf": "protobuflinux11",
    "cpython": "pythonlinux11",
    "sul": "sullinux11",
    "thrift": "thriftlinux11",
    "xzutils": "xzutilslinux",
    "libzmq": "zeromqlinux11",
    "zlib": "zliblinux11"
}

def find_all_release_xmls(start_dir):
    release_package_names = ["release-package-linux.xml", "linux-release-package.xml", "linux_package.xml",
                             "sed-libs-linux-package.xml", "linux-centos7.xml", "dev.xml", "sdds3.xml",
                             "rtd-package.xml", "release-package.xml", "package.xml"]
    release_file_locations = []

    for repo in next(os.walk(start_dir))[1]:
        package_found = False
        for release_package_name in release_package_names:
            if not package_found:
                for filename in Path(os.path.join(start_dir, repo)).rglob(release_package_name):
                    package_found = True
                    release_file_locations.append(filename)
    return release_file_locations


def release_package_contains_inputs(release_package_dict):
    if "inputs" in release_package_dict['package']:
        if release_package_dict['package']["inputs"] is not None:
            if "build-asset" in release_package_dict['package']['inputs']:
                return True
    return False


def process_release_files(release_files):
    all_dict = {}
    for release_file_path in release_files:
        with open(release_file_path, 'r') as release_file:
            print(f"\nProcessing {release_file_path}")

            content = release_file.read()
            as_dictionary = xmltodict.parse(content, dict_constructor=dict)

            if "package" not in as_dictionary:
                print(f"Skipping invalid release package: {release_file_path}")
                continue

            if "name" in as_dictionary['package']:
                name = as_dictionary['package']['name']
            elif "@name" in as_dictionary['package']:
                name = as_dictionary['package']['@name']
            else:
                print(f"Skipping input as the release-package is not formatted correctly: {as_dictionary}")
                continue

            if "@version" not in as_dictionary['package']:
                print(f"Skipping input as the release-package is not formatted correctly: {as_dictionary}")
                continue

            version = as_dictionary['package']['@version'].replace("-", ".")

            # Check the component has inputs
            if release_package_contains_inputs(as_dictionary) is False:
                print(f"Skipping: {name} as it has no inputs")
                continue

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
                    print(f"Skipping input: {input_pkg['@repo']} in: {release_file_path}, as there is no release-version")
                    continue

                input_dict = {"name": repoNameToPackageName[input_pkg["@repo"]],
                              "repo_name": input_pkg["@repo"],
                              "branch": input_pkg["release-version"]["@branch"],
                              "build_id": input_pkg["release-version"]["@build-id"],
                              "artifacts": []}

                if type(input_pkg['include']) is dict:
                    input_dict["artifacts"].append(input_pkg['include'])
                elif type(input_pkg['include']) is list:
                    for artifact in input_pkg['include']:
                        input_dict["artifacts"].append(artifact)

                print(input_dict["name"])
                print(input_dict["branch"])
                print(input_dict["build_id"])
                print(input_dict['artifacts'])

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
