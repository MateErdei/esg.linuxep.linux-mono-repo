#!/usr/bin/env python3
import grp
import json
import os
import shutil
import sys
import time
import traceback
from pathlib import Path
from pwd import getpwnam
from random import randrange

from watchdog.events import PatternMatchingEventHandler
from watchdog.observers import Observer

from performance.PerformanceResources import SOPHOS_INSTALL, stop_sspl_process, get_current_unix_epoch_in_seconds, \
    start_sspl_process

RESPONSE_COUNT = 0
TMP_ACTIONS_DIR = os.path.join("/tmp", "actions")
BASE_ACTION_DIR = os.path.join(SOPHOS_INSTALL, "base", "mcs", "action")
os.makedirs(TMP_ACTIONS_DIR, exist_ok=True)


class FilesystemWatcher(object):
    def __init__(self):
        self.ignore_patterns = []

    def setup_filesystem_watcher(self, path, match_patterns=None, ignore_patterns=None, ignore_directories=True):
        if match_patterns is None:
            match_patterns = ["*"]

        self.patterns = match_patterns
        if ignore_patterns is not None:
            self.ignore_patterns = ignore_patterns

        # self.ignore_patterns.append(f"*{self.log_file_path}")
        self.ignore_directories = ignore_directories
        self.case_sensitive = True
        self.event_handler = PatternMatchingEventHandler(self.patterns, self.ignore_patterns, self.ignore_directories, self.case_sensitive)
        self.path = path
        self.go_recursively = True
        self.observer = Observer()
        self.observer.schedule(self.event_handler, self.path, recursive=self.go_recursively)

    def start_filesystem_watcher(self):
        self.observer.start()

    def stop_filesystem_watcher(self):
        self.observer.stop()
        self.observer.join()


def make_file_readable_by_mcs(file_path):
    uid = getpwnam('sophos-spl-user').pw_uid
    gid = grp.getgrnam('sophos-spl-group')[2]
    os.chown(file_path, uid, gid)


def run_live_query(query, name):
    random_correlation_id = f"correlation-id-{randrange(10000000)}"
    query_json = '{"type": "sophos.mgt.action.RunLiveQuery", "name": "' + name + '", "query": "' + query + '"}'
    query_file_name = f"LiveQuery_{random_correlation_id}_request_2020-01-01T09:50:08Z_1698305948.json"
    query_file_path = os.path.join(TMP_ACTIONS_DIR, query_file_name)
    with open(query_file_path, 'w') as action_file:
        action_file.write(query_json)
        make_file_readable_by_mcs(query_file_path)

    # Move query file into mcs action dir to be picked up by management agent
    shutil.move(query_file_path, BASE_ACTION_DIR)


def inc_response_count(event):
    global RESPONSE_COUNT
    RESPONSE_COUNT += 1


def remove_all_pending_responses():
    response_dir = os.path.join(SOPHOS_INSTALL, "base", "mcs", "response")
    for p in Path(response_dir).glob("*.json"):
        p.unlink()


def run_query_n_times_and_wait_for_responses(query_name, query_string, times_to_send):
    global RESPONSE_COUNT

    end_time = 0
    start_time = 0
    try:
        # We get blacklisted from central if bad live query responses go up
        stop_sspl_process('mcsrouter')
        response_dir = os.path.join(SOPHOS_INSTALL, "base", "mcs", "response")
        fsw = FilesystemWatcher()
        fsw.setup_filesystem_watcher(response_dir, match_patterns=["*.json"])
        # Add callback for on created, note fsw.event_handler has other events too, e.g. fsw.event_handler.on_moved
        fsw.event_handler.on_created = inc_response_count
        fsw.start_filesystem_watcher()

        start_time = get_current_unix_epoch_in_seconds()
        for i in range(0, times_to_send):
            run_live_query(query_string, query_name)

        # Seconds from now.
        timeout = time.time() + 100
        while RESPONSE_COUNT < times_to_send and time.time() < timeout:
            time.sleep(0.01)

        end_time = get_current_unix_epoch_in_seconds()

        fsw.stop_filesystem_watcher()
        remove_all_pending_responses()
    except Exception as ex:
        print(f"Failed: {ex}")
        traceback.print_exc(file=sys.stdout)
    finally:
        # Make sure we start mcsrouter up again
        start_sspl_process('mcsrouter')

    result = {"response_count": RESPONSE_COUNT,
              "times_to_send": times_to_send,
              "start_time": start_time,
              "end_time": end_time,
              "duration": end_time - start_time,
              "success": RESPONSE_COUNT == times_to_send}

    return result


def print_usage_and_exit(script_name):
    print(f"Usage: {script_name} <query name> <query string>")
    print(f'Example: {script_name} "my test query" "select * from users;"')
    print("OR")
    print(f"Usage: {script_name} <query name> <query string> <number of times to run query>")
    print(f'Example: {script_name} "my test query" "select * from users;" 5')
    exit()


def main(args):
    number_of_args = len(args) - 1

    if number_of_args == 1:
        if args[1] == "--help" or args[1] == "-h":
            print_usage_and_exit(args[0])

    if number_of_args != 2 and number_of_args != 3:
        print_usage_and_exit(args[0])

    query_name = args[1]
    query_string = args[2]
    times_to_send = 1
    if number_of_args == 3:
        times_to_send = int(args[3])

    result = run_query_n_times_and_wait_for_responses(query_name, query_string, times_to_send)
    print(json.dumps(result))
    return result["success"]


if __name__ == '__main__':
    main(sys.argv)
