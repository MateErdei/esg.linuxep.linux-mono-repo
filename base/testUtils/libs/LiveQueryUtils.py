import json
import os
import grp
import shutil
from pwd import getpwnam
from random import randrange

import BaseInfo as base_info

TMP_ACTIONS_DIR = os.path.join("/tmp", "actions")
BASE_ACTION_DIR = os.path.join(base_info.get_install(), "base", "mcs", "action")
os.makedirs(TMP_ACTIONS_DIR, exist_ok=True)

def verify_livequery_request_has_the_expected_fields(filepath, **kwargs):
    with open(filepath, 'r') as query_file:
            query_dict = json.load(query_file)

    if query_dict != kwargs:
        # keys must be the same
        kwargs_keys = set(kwargs.keys())
        query_dict_keys = set(query_dict.keys())
        if kwargs_keys != query_dict_keys:
            only_in_query_dict = query_dict_keys.difference(kwargs_keys)
            only_in_kwargs = kwargs_keys.difference(query_dict_keys)
            raise AssertionError(
               """Mismatch of json keys. Present in the arguments only: {}. Present in the json file only: {}.
Json File Content: {}""".format(
                   only_in_kwargs, only_in_query_dict, query_dict))
        for key in kwargs_keys:
            kwarg_value = kwargs.get(key)
            json_value = query_dict.get(key)
            if kwarg_value != json_value:
                raise AssertionError(
                   "Value differ for key {}. Expected {}. In json file: {}\n. Json File Content: {}".format(
                      key, kwarg_value, json_value, query_dict))

def make_file_readable_by_mcs(file_path):
    uid = getpwnam('sophos-spl-user').pw_uid
    gid = grp.getgrnam('sophos-spl-group')[2]
    os.chown(file_path, uid, gid)

def run_live_query(query, name):
    random_correlation_id = "correlation-id-{}".format(randrange(10000000))
    query_json = '{"type": "sophos.mgt.action.RunLiveQuery", "name": "' + name + '", "query": "' + query + '"}'
    query_file_name = "LiveQuery_{}_request_2013-05-02T09:50:08Z_1692398393.json".format(random_correlation_id)
    query_file_path = os.path.join(TMP_ACTIONS_DIR, query_file_name)
    with open(query_file_path, 'w') as action_file:
        action_file.write(query_json)
        make_file_readable_by_mcs(query_file_path)

    # Move query file into mcs action dir to be picked up by management agent
    shutil.move(query_file_path, BASE_ACTION_DIR)

def get_correlation_id():
    import uuid
    return str(uuid.uuid4())
