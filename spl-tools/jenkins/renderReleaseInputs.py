#!/usr/bin/env python3

import os
import json
from graphviz import Digraph
dot = Digraph(comment='Dependencies', engine="dot", format='png')

json_file_source = "releaseInputs.json"
json_string = ""
if os.path.isfile(json_file_source):
    with open(json_file_source, 'r') as f:
        json_string = f.read()

if json_string == "":
    exit(1)

root_dictionary = json.loads(json_string)

all_inputs = {}

dot.attr(rankdir='LR')

dot.node("KEY - SSPL Components", color='blue', shape='doublecircle')
dot.node("KEY - Repos we build", color='blue')

for component_name, info_dict in root_dictionary.items():
    component_id = f"{component_name} ({info_dict['version']})"

    if "sspl" in component_id.lower():
        dot.node(component_id, color='blue', shape='doublecircle')
    else:
        dot.node(component_id, color='blue')

    for i in info_dict["inputs"]:
        input_id = f"{i['name']} ({i['build_id']})"
        dot.edge(input_id, component_id)

dot.attr('graph', overlap='false')

# Save as png image
dot.attr('graph', format='png')
# If you run this locally you can change view to be true for it to open.
dot.render("dependencies", view=False)

# Save as pdf
dot.attr('graph', format='pdf')
dot.render("dependencies.pdf", view=False)
