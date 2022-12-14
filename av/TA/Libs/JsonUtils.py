
import json


def load_json_from_file(path):
    return json.load(open(path))


def check_json_contains(json_object, key, value):
    if json_object.get(key, None) != value:
        raise AssertionError("JSON: %s doesn't contain [%s]=%s" %
                             (
                                 json.dumps(json_object),
                                 key,
                                 value
                             ))
