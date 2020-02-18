#!/usr/bin/env python3
import grp
import os
import shutil
import sys
import time
from random import randrange
from pwd import getpwnam
import subprocess
from watchdog.observers import Observer
from watchdog.events import PatternMatchingEventHandler
from pathlib import Path

RESPONSE_COUNT = 0
SOPHOS_INSTALL = "/opt/sophos-spl"
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

        # self.ignore_patterns.append("*{}".format(self.log_file_path))
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
    global TMP_ACTIONS_DIR
    global BASE_ACTION_DIR
    random_correlation_id = "correlation-id-{}".format(randrange(10000))
    query_json = '{"type": "sophos.mgt.action.RunLiveQuery", "name": "' + name + '", "query": "' + query + '"}'
    query_file_name = "LiveQuery_{}_2013-05-02T09:50:08Z_request.json".format(random_correlation_id)
    query_file_path = os.path.join(TMP_ACTIONS_DIR, query_file_name)
    with open(query_file_path, 'w') as action_file:
        action_file.write(query_json)
        make_file_readable_by_mcs(query_file_path)

    # Move query file into mcs action dir to be picked up by management agent
    shutil.move(query_file_path, BASE_ACTION_DIR)


def inc_response_count(event):
    global RESPONSE_COUNT
    print("Response created: {}".format(event.src_path))
    RESPONSE_COUNT += 1


def stop_mcsrouter():
    global SOPHOS_INSTALL
    wdct_path = os.path.join(SOPHOS_INSTALL, "bin", "wdctl")
    subprocess.run([wdct_path, "stop", "mcsrouter"])


def start_mcsrouter():
    global SOPHOS_INSTALL
    wdct_path = os.path.join(SOPHOS_INSTALL, "bin", "wdctl")
    subprocess.run([wdct_path, "start", "mcsrouter"])


def remove_all_pending_responses():
    global SOPHOS_INSTALL
    response_dir = os.path.join(SOPHOS_INSTALL, "base", "mcs", "response")
    for p in Path(response_dir).glob("*.json"):
        p.unlink()


def main(argv):
    global RESPONSE_COUNT
    global SOPHOS_INSTALL
    number_of_queries = 3
    try:
        # We get blacklisted from central if bad live query responses go up
        stop_mcsrouter()
        response_dir = os.path.join(SOPHOS_INSTALL, "base", "mcs", "response")
        fsw = FilesystemWatcher()
        fsw.setup_filesystem_watcher(response_dir, match_patterns=["*.json"])
        # Add callback for on created, note fsw.event_handler has other events too, e.g. fsw.event_handler.on_moved
        fsw.event_handler.on_created = inc_response_count
        fsw.start_filesystem_watcher()

        for i in range(0, number_of_queries):
            run_live_query("select * from users;", "perf-query-name")

        # Seconds from now.
        timeout = time.time() + 100
        while RESPONSE_COUNT < number_of_queries and time.time() < timeout:
            time.sleep(0.01)

        fsw.stop_filesystem_watcher()
        remove_all_pending_responses()
    except Exception as ex:
        print("Failed: {}".format(ex))
    finally:
        # Make sure we start mcsrouter up again
        start_mcsrouter()

    return RESPONSE_COUNT == number_of_queries


if __name__ == '__main__':
    main(sys.argv)
