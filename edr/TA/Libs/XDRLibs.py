#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import json
import time

def linux_queries_in_pack(config: dict)  -> (str, dict):
    # Only yield linux queries
    for query_name, query in queries_in_pack(config):
        # queries without designated platforms will run on linux
        if "linux" in query.get("platform", "linux"):
            yield query_name, query

def queries_in_pack(config: dict) -> (str, dict):
    # for query in flat "schedule" field
    for query_name, query in config["schedule"].items():
        yield query_name, query
    #for query in packs
    for pack in config.get("packs", {}).values():
        for query_name, query in pack["queries"].items():
            yield query_name, query

def replace_query_bodies_with_sql_that_always_gives_results(config_path):
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            config_json_string = f.read()
        config = json.loads(config_json_string)

        query_that_always_returns = "SELECT CURRENT_TIMESTAMP AS current_date_time;"
        for query_name, query_dict in linux_queries_in_pack(config):
            print(f"changing {query_name}'s query field to {query_that_always_returns}")
            query_dict["query"] = query_that_always_returns

        new_config_json_string = json.dumps(config, indent=4)
        with open(config_path, 'w') as f:
            f.write(new_config_json_string)
    else:
        raise AssertionError(f"{config_path} does not exist")


def change_all_scheduled_queries_interval(config_path, interval):
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            config_json_string = f.read()
        config = json.loads(config_json_string)
        # edit flat queries under "schedule"
        for query_name, query in config["schedule"].items():
            try:
                query["interval"] = interval
                query["blacklist"] = False
            except Exception as e:
                print("No interval for: " + query_name)
                pass
        #edit queries in packs
        for pack in config.get("packs", {}).values():
            for query_name, query in pack["queries"].items():
                try:
                    query["interval"] = interval
                    query["blacklist"] = False
                except Exception as e:
                    print(f"No interval for: {query_name}")
                    pass

        new_config_json_string = json.dumps(config, indent=4)
        with open(config_path, 'w') as f:
            f.write(new_config_json_string)
    else:
        raise AssertionError(f"{config_path} does not exist")

def remove_discovery_query_from_pack(config_path):
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            config_json_string = f.read()
        config = json.loads(config_json_string)

        for pack in config.get("packs", {}).values():
            pack.pop('discovery', None)

        new_config_json_string = json.dumps(config, indent=4)
        with open(config_path, 'w') as f:
            f.write(new_config_json_string)
    else:
        raise AssertionError(f"{config_path} does not exist")

def check_all_queries_run(log_path: str, config_path: str):
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            config_json_string = f.read()
        config = json.loads(config_json_string)
        found = 0

        flattened_queries = {}
        packs = config.get("packs", {})

        for pack in packs.values():
            flattened_queries = {**flattened_queries, **pack["queries"]}

        flattened_queries = {**flattened_queries, **config["schedule"]}

        for query_name in flattened_queries:
            if "platform" in flattened_queries[query_name] and "linux" not in flattened_queries[query_name]["platform"]:
                print(f"Skipping {query_name} because it's specified platforms does not include linux")
            else:
                print(f"checking for {query_name}")
                check_for_query_in_log(log_path,  query_name)
                print("Found: " + query_name)
                found += 1
        if found == 0:
            raise AssertionError("Did not search for any queries, are you sure this keyword is testing what you think it is?")
    else:
        raise AssertionError(f"{config_path} does not exist")


def check_for_query_in_log(log_path, query_name: str):
    if os.path.exists(log_path):
        with open(log_path, 'r') as f:
            log_content = f.read()
    log_content_stripped = log_content.replace("\n", "")
    if query_name not in log_content_stripped:
        raise AssertionError("could not find query in log: " + query_name)

def check_all_query_results_contain_correct_tag(results_directory: str, *config_paths):
    assert(len(config_paths) > 0)

    results_dict = {}
    for file in os.listdir(results_directory):
        print(f"file: {file}")
        with open(os.path.join(results_directory, file), 'r') as f:
            results_json_string = f.read()

        results_json = json.loads(results_json_string)
        for result in results_json:
            results_dict[result["name"]] = result

    for path in config_paths:
        if not os.path.exists(path):
            raise AssertionError(f"{path} does not exist")
        with open(path, 'r') as f:
            config_json_string = f.read()
        config = json.loads(config_json_string)

        for query_name, query_dict in linux_queries_in_pack(config):
            print(f"Checking {query_name}")
            assert(query_dict["tag"] == results_dict[query_name]["tag"])


def check_query_results_folded(query_result: str, expected_query: str, expected_column_name: str, expected_column_value: str):
    query_json = json.loads(query_result)
    is_folded = False
    if isinstance(query_json, list):
        if len(query_json) == 0:
            raise AssertionError("no queries")
        query_found = False
        for result in query_json:
            if result['name'] == expected_query and expected_column_name in result['columns']:
                if result['columns'][expected_column_name] == expected_column_value:
                    query_found = True
                    if 'folded' in result and result['folded'] > 1:
                        is_folded = True
        if not query_found:
            raise AssertionError(f"query {expected_query} not found")
    else:
        raise AssertionError("query not list")
    return is_folded


def integer_is_within_range(integer, lower, upper):
    assert int(lower) <= int(integer) <= int(upper), f"expected {lower} <= {integer} <= {upper}"

def get_current_epoch_time():
    return int(time.time())

def wait_for_scheduled_query_file_and_return_filename():
    n = 0
    while n < 60:
        files = os.listdir("/opt/sophos-spl/base/mcs/datafeed")
        if len(files) == 0:
            time.sleep(1)
            n += 1
            next
        else:
            return files[0]
    raise AssertionError("Did not find scheduled query datafeed file")