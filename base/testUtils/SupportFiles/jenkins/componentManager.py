#!/usr/bin/python3

import json
import testUtils
import os

THIS_FILE_PATH = os.path.realpath(__file__)
dir_path = os.path.dirname(THIS_FILE_PATH)
build_input_file = os.path.join(dir_path, "buildInputs.json")

class ComponentManager:
    def __init__(self, input_file_path):
        json.load(input_file_path)



if __name__ == '__main__':
    print("hello world")