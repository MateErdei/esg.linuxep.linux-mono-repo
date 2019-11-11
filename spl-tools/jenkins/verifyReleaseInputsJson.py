#!/usr/bin/env python3

from pathlib import Path
import os
import xmltodict
import sys
import json


def verify_json(json_string):
    as_dict = json.loads(json_string)
    for component_name, info_dict in as_dict.items():
        print(component_name)
        print(info_dict)


def main(argv):

    json_string = ""

    # URL at time of writing: "https://10.55.36.24/job/Generate-Release-Package-Input-Data/HTML_20Report/releaseInputs.json"
    url_to_json = None
    if len(argv) > 1:
        url_to_json = argv[1]

    # If no url specified then rty reading local file as this may be running manually.
    if url_to_json is None:
        json_file_source = "releaseInputs.json"
        if os.path.isfile(json_file_source):
            with open(json_file_source, 'r') as f:
                json_string = f.read()
    else:
        import requests
        r = requests.get(url=url_to_json)
        json_string = r.json()


    verify_json(json_string)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
