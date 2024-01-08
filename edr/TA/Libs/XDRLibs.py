#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020-2023 Sophos Limited. All rights reserved.
# All rights reserved.

import os
import json
import time
from robot.api import logger


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
        discovery = pack.get("discovery", "")
        logger.debug("Discovery: {}".format(discovery))
        # Exclude queries which only run on Windows
        if "osquery_registry" not in str(discovery):
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
            discovery = pack.get("discovery", "")
            logger.debug("Discovery: {}".format(discovery))
            # Exclude queries which only run on Windows
            if "osquery_registry" not in str(discovery):
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


def convert_canned_query_json_to_query_pack(json_path, query_pack_path):
    if os.path.isfile(json_path):
        with open(json_path, 'r') as f:
            config_json_string = f.read()
    else:
        raise AssertionError(f"{json_path} does not exist!")

    canned_json = json.loads(config_json_string)
    canned_pack = {"schedule": {}}
    for query in canned_json["queries"]:
        if "linuxServer" in query["platforms"]:
            try:
                if "mtr" in query["requires"]:
                    break
            except KeyError:
                ## if no requires then it is not an mtr query
                pass
            query_json = {
                "interval": 20,
                "query": query["query"]
            }
            canned_pack["schedule"][query["id"]] = query_json

    with open(query_pack_path, 'w') as f:
        json.dump(canned_pack, f)

    assert os.path.isfile(query_pack_path)
    logger.info(f"{query_pack_path} created")


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
        logger.info(f"File: {file}")
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
            logger.info(f"Checking {query_name}")
            assert(query_dict["tag"] == results_dict[query_name]["tag"])


def check_query_results_folded(query_result: str, expected_query: str, expected_column_name: str, expected_column_value: str = None):
    query_json = json.loads(query_result)
    if isinstance(query_json, list):
        if len(query_json) == 0:
            raise AssertionError("no queries")
        query_found = False
        for result in query_json:
            if result['name'] == expected_query and expected_column_name in result['columns']:
                actual_column_value = str(result['columns'][expected_column_name])
                print(f"result['columns'][{expected_column_name}] = {actual_column_value}")
                if expected_column_value is not None and actual_column_value != expected_column_value:
                    continue
                query_found = True
                if 'folded' in result and result['folded'] > 0:
                    return True
        if not query_found:
            raise AssertionError(f"query {expected_query} not found in: {query_result}")
    else:
        raise AssertionError("query not list")
    return False


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

def clear_datafeed_folder():
    files = os.listdir("/opt/sophos-spl/base/mcs/datafeed")
    for file in files:
        os.remove(os.path.join("/opt/sophos-spl/base/mcs/datafeed", file))


def print_query_result_as_table(query_result):
    query_result_json = json.loads(query_result)
    headings = query_result_json["columnMetaData"]
    results = query_result_json["columnData"]

    heading_widths = []
    for h in headings:
        heading_widths.append(len(str(h["name"])))

    max_result_column_widths = []
    for row in results:
        column = 0
        for value in row:
            val_str = str(value)
            if len(max_result_column_widths) <= column:
                max_result_column_widths.append(len(val_str))
            elif len(val_str) > max_result_column_widths[column]:
                max_result_column_widths[column] = len(val_str)
            column += 1

    heading_row = ""
    result_rows = []

    curr_column = 0
    for h in headings:
        heading_row += f"{h['name']:<{1 + max(heading_widths[curr_column], max_result_column_widths[curr_column])}}"
        curr_column += 1

    for row in results:
        curr_column = 0
        row_str = ""
        for value in row:
            row_str += f"{value:<{1 + max(heading_widths[curr_column], max_result_column_widths[curr_column])}}"
            curr_column += 1
        result_rows.append(row_str)

    # Print the table
    table_str = heading_row + "\n"
    table_str += "\n".join(result_rows)
    logger.info(table_str)

