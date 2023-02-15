#!/usr/bin/env python3

import os
import json
from graphviz import Digraph


def format_name(name):
    names_to_format = {
        "esg": "Mono Repo",
        "everest-base": "Base",
        "runtimedetections": "RTD",
        "sspl-plugin-anti-virus": "AV",
        "sspl-plugin-edr-component": "EDR",
        "sspl-plugin-event-journaler": "Event Journaler",
        "sspl-plugin-mdr-component": "MTR",
        "sspl-warehouse": "SSPL\nWarehouse",
    }
    return names_to_format[name] if name in names_to_format else name


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

for component_name, info_dict in root_dictionary.items():
    component_id = format_name(component_name)
    dot.node(component_id, color='blue', shape='doublecircle')

    for i in info_dict["inputs"]:
        input_id = format_name(i['name'])
        dot.edge(input_id, component_id)

dot.attr('graph', overlap='false')

# Save as png image
dot.attr('graph', format='png')
# If you run this locally you can change view to be true for it to open.
dot.render("dependencies", view=False)

# Save as pdf
dot.attr('graph', format='pdf')
dot.render("dependencies.pdf", view=False)
