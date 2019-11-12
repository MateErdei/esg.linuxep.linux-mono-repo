#!/usr/bin/env python3

import os
import json
from graphviz import Digraph
dot = Digraph(comment='Dependencies', engine="neato", format='png')

json_file_source = "releaseInputs.json"
json_string = ""
if os.path.isfile(json_file_source):
    with open(json_file_source, 'r') as f:
        json_string = f.read()

if json_string == "":
    exit(1)

root_dictionary = json.loads(json_string)

all_inputs = {}

for component_name, info_dict in root_dictionary.items():
    component_id = "{} ({})".format(component_name, info_dict["version"])
    dot.node(component_id)

    for i in info_dict["inputs"]:
        input_id = "{} ({})".format(i["name"], i["version"])
        dot.node(input_id)
        dot.edge(input_id, component_id)


dot.attr('graph', overlap='false')

# If you run this lcoally you can change view to be true for it to open.
dot.render("dependencies", view=False)
