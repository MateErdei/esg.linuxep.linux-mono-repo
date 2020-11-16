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


def check_all_queries_run(log_path: str, config_path: str):
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            config_json_string = f.read()
        config = json.loads(config_json_string)
        for query_name in config["schedule"]:
            if "platform" in config["schedule"][query_name] and config["schedule"][query_name]["platform"] == "linux":
                check_for_query_in_log(log_path,  query_name)
                print("Found: " + query_name)


def check_for_query_in_log(log_path, query_name: str):
    if os.path.exists(log_path):
        with open(log_path, 'r') as f:
            log_content = f.read()
    log_content_stripped = log_content.replace("\n", "")
    if query_name not in log_content_stripped:
        raise AssertionError("could not find query in log: " + query_name)

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