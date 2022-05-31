#!/usr/bin/env python3
from __future__ import absolute_import, print_function, division, unicode_literals

import json
import os


def get_instance_json_as_string(instance_name):
    path_to_json = os.path.join("./instances", instance_name + ".json")
    with open(path_to_json) as instanceJsonFile:
        data = json.loads(instanceJsonFile.read())[instance_name]
        return json.dumps(data)


EXCLUDED = []
RUNONE = os.environ.get("RUNONE", "")


def load_instances():
    result = {}
    for f in os.listdir("./instances"):
        base, ext = os.path.splitext(f)
        assert ext == ".json"
        if base in EXCLUDED and base != RUNONE:
            print("!!! Excluding %s !!!" % base)
            continue

        result[base] = get_instance_json_as_string(base)
    return result


instances = load_instances()


def args():
    if os.path.isfile("argFile"):
        with open("argFile") as arg_file:
            arg_lines = arg_file.readlines()
        for arg_line in arg_lines:
            if '"' in arg_line or "'" in arg_line:
                raise AssertionError('We DO NOT support quotes in these args. If you MUST use a robot argument which \
                                      contains whitespace, please swap it for underscores as robot treats them equally')

            include, *excludes = arg_line.rstrip().split("NOT")
            arg_vals = ['-i', include]
            for exclude in excludes:
                arg_vals.extend(['-e', exclude])
            yield " ".join(arg_vals)
    else:
        yield ""


def main():
    with open("sspl-system.template") as main_template_file:
        main_template_json = json.loads(main_template_file.read())

    for index, arguments in enumerate(args(), 1):
        print("Adding templates with args for: " + arguments)
        for template_name, template_json_str in instances.items():
            unique_template_name = template_name + "slice" + str(index)
            hostname = template_name + "-" + str(index)
            json_with_args = json.loads(template_json_str.replace("@ARGSGOHERE@", arguments)
                                                         .replace("@HOSTNAMEGOESHERE@", hostname))
            main_template_json["Resources"][unique_template_name] = json_with_args

    with open("sspl-system.template.with_args", "w") as outFile:
        outFile.write(json.dumps(main_template_json, indent=4))


if __name__ == '__main__':
    main()
