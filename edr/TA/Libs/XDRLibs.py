#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import json


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

        for mtr_query_name in config["packs"]["mtr"]["queries"]:
            try:
                config["packs"]["mtr"]["queries"][mtr_query_name]["interval"] = interval
                config["packs"]["mtr"]["queries"][mtr_query_name]["blacklist"] = False
            except Exception as e:
                print("No interval for: " + mtr_query_name)
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
