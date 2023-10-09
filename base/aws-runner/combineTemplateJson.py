#!/usr/bin/env python

import json
import os

def getInstanceJsonAsString(instanceName):
    path_to_json = os.path.join("./instances", instanceName + ".json")
    with open(path_to_json) as instanceJsonFile:
        dict = json.loads(instanceJsonFile.read())[instanceName]

    return json.dumps(dict)

instances = {
    "amazonlinux2x64": getInstanceJsonAsString("amazonlinux2x64"),
    "amazonlinux2022x64": getInstanceJsonAsString("amazonlinux2022x64"),
    "centos7x64": getInstanceJsonAsString("centos7x64"),
    "centosstreamx64": getInstanceJsonAsString("centosstreamx64"),
    "debian10x64": getInstanceJsonAsString("debian10x64"),
    "debian11x64": getInstanceJsonAsString("debian11x64"),
    "miraclelinuxx64": getInstanceJsonAsString("miraclelinuxx64"),
    "oracle7x64": getInstanceJsonAsString("oracle7x64"),
    "oracle8x64": getInstanceJsonAsString("oracle8x64"),
    "rhel78x64": getInstanceJsonAsString("rhel78x64"),
    "rhel81x64": getInstanceJsonAsString("rhel81x64"),
    "rhel9x64": getInstanceJsonAsString("rhel9x64"),
    "ubuntu1804minimal": getInstanceJsonAsString("ubuntu1804minimal"),
    "ubuntu1804x64": getInstanceJsonAsString("ubuntu1804x64"),
    "ubuntu2004": getInstanceJsonAsString("ubuntu2004"),
    "ubuntu2204": getInstanceJsonAsString("ubuntu2204")
}

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
            unique_template_name = template_name + "load" +str(n)
            json_with_args = json.loads(template_json_str.replace("@ARGSGOHERE@", arguments).replace("@HOSTNAMEGOESHERE@", unique_template_name))
            main_template_json["Resources"][unique_template_name] = json_with_args
        n+=1

    with open("sspl-system.template.with_args", "w") as outFile:
        outFile.write(json.dumps(main_template_json, indent=4))

if __name__ == '__main__':
    main()