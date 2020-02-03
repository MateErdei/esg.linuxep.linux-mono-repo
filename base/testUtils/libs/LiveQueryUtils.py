import json


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
