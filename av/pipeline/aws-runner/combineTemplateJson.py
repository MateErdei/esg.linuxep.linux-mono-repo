#!/usr/bin/env python3
from __future__ import absolute_import, print_function ,division, unicode_literals

import json
import os

def getInstanceJsonAsString(instanceName):
    path_to_json = os.path.join("./instances", instanceName + ".json")
    with open(path_to_json) as instanceJsonFile:
        dict = json.loads(instanceJsonFile.read())[instanceName]
        return json.dumps(dict)


def load_instances():
    result = {}
    for f in os.listdir("./instances"):
        base, ext = os.path.splitext(f)
        assert ext == ".json"
        result[base] = getInstanceJsonAsString(base)
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
            yield arg_line.rstrip()
    else:
        yield ""

def main():
    with open("sspl-system.template") as main_template_file:
        main_template_json = json.loads(main_template_file.read())

    n = 1
    for arguments in args():
        print("Adding templates with args for: " + arguments)
        for template_name, template_json_str in instances.items():
            unique_template_name = template_name + str(n)
            json_with_args = json.loads(template_json_str.replace("@ARGSGOHERE@", arguments)
                                                         .replace("@HOSTNAMEGOESHERE@", unique_template_name))
            main_template_json["Resources"][unique_template_name] = json_with_args
        n += 1

    with open("sspl-system.template.with_args", "w") as outFile:
        outFile.write(json.dumps(main_template_json, indent=4))


if __name__ == '__main__':
    main()