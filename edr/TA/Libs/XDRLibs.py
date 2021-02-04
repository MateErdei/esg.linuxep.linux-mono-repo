#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import json
import time


def change_all_scheduled_queries_interval(config_path, interval):
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            config_json_string = f.read()
        config = json.loads(config_json_string)
        for query_name in config["schedule"]:
            try:
                config["schedule"][query_name]["interval"] = interval
                config["schedule"][query_name]["blacklist"] = False
            except Exception as e:
                print("No interval for: " + query_name)
                pass

        new_config_json_string = json.dumps(config, indent=4)
        with open(config_path, 'w') as f:
            f.write(new_config_json_string)


def check_all_queries_run(log_path: str, config_path: str, custom_pack=False):
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            config_json_string = f.read()
        config = json.loads(config_json_string)
        found = 0
        for query_name in config["schedule"]:
            if "platform" in config["schedule"][query_name] and "linux" not in config["schedule"][query_name]["platform"]:
                print(f"Skipping {query_name} because it's specified platforms does not include linux")
            else:
                print(f"checking for {query_name}")
                check_for_query_in_log(log_path,  query_name)
                print("Found: " + query_name)
                found += 1
        if found == 0:
            raise AssertionError("Did not search for any queries, are you sure this keyword is testing what you think it is?")
    else:
        raise AssertionError("Failed to r"
                             "ead config")


def check_for_query_in_log(log_path, query_name: str):
    if os.path.exists(log_path):
        with open(log_path, 'r') as f:
            log_content = f.read()
    log_content_stripped = log_content.replace("\n", "")
    if query_name not in log_content_stripped:
        raise AssertionError("could not find query in log: " + query_name)

def check_all_query_results_contain_correct_tag(results_directory: str, config_path1: str,  config_path2: str, expected_tag: str):
    if os.path.exists(config_path1) and os.path.exists(config_path2):
        with open(config_path1, 'r') as f:
            config_json_string1 = f.read()
        config1 = json.loads(config_json_string1)
        with open(config_path2, 'r') as f:
            config_json_string2 = f.read()
        config2 = json.loads(config_json_string2)

        config = {**config1["schedule"], **config2["schedule"]}

        results_json = json.loads("[]")
        for file in os.listdir(results_directory):
            print(f"file: {file}")
            with open(os.path.join(results_directory, file), 'r') as f:
                results_json_string = f.read()
            results_json += json.loads(results_json_string)
        for result in results_json:
            if config[result["name"]]["tag"] != result["tag"]:
                raise AssertionError("tags do not match")

def integer_is_within_range(integer, lower, upper):
    assert int(lower) <= int(integer) <= int(upper), f"expected {lower} <= {integer} <= {upper}"

def get_current_epoch_time():
    return int(time.time())

def wait_for_scheduled_query_file_and_return_filename():
    n = 0
    while n < 30:
        files = os.listdir("/opt/sophos-spl/base/mcs/datafeed")
        if len(files) == 0:
            time.sleep(1)
            n += 1
            next
        else:
            return files[0]
    raise AssertionError("Did not find scheduled query datafeed file")