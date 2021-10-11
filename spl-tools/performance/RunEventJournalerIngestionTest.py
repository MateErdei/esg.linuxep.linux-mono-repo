#!/usr/bin/env python3
import json
import os
import shutil
import subprocess
import sys
import time

SOPHOS_INSTALL = "/opt/sophos-spl"


def stop_eventjournaler():
    wdctl_path = os.path.join(SOPHOS_INSTALL, "bin", "wdctl")
    subprocess.run([wdctl_path, "stop", "eventjournaler"])


def start_eventjournaler():
    wdctl_path = os.path.join(SOPHOS_INSTALL, "bin", "wdctl")
    subprocess.run([wdctl_path, "start", "eventjournaler"])


def get_current_unix_epoch_in_seconds():
    return time.time()


def run_ingestion_test(number_of_events_to_send):
    start_time = 0
    end_time = 0

    journal_dir = "/opt/sophos-spl/plugins/eventjournaler/data/eventjournals"
    detections_dir = f"{journal_dir}/SophosSPL/Detections"
    tmp_detections_dir = "/tmp/Detections"

    shutil.rmtree(tmp_detections_dir, ignore_errors=True)

    if os.path.exists(detections_dir):
        stop_eventjournaler()
        shutil.move(detections_dir, tmp_detections_dir)
        start_eventjournaler()
        # Allow event journaler some time to start.
        time.sleep(5)

    try:
        print(f"Sending {number_of_events_to_send} events")

        start_time = get_current_unix_epoch_in_seconds()
        command = ['/root/performance/EventPubSub',
                   '-s', "/opt/sophos-spl/var/ipc/events.ipc",
                   '-c', number_of_events_to_send,
                   '-z',  # go fast and don't use sleep in the send loop i.e. send events as fast as possible.
                   'send']
        process_result = subprocess.run(command, timeout=120, stdout=subprocess.PIPE, encoding="utf-8")
        assert(process_result.returncode == 0)
        end_time = get_current_unix_epoch_in_seconds()

        # Allow the event journaler buffer to empty
        time.sleep(5)

        event_count = 0
        command = ['/root/performance/JournalReader',
                   '-l', journal_dir,
                   '-s', 'Detections',
                   '-t', '0',
                   '--stats', 'view',
                   '-hd', 'none',
                   '--flush-delay-disable']
        process_result = subprocess.run(command, timeout=120, stdout=subprocess.PIPE, encoding="utf-8")
        for line in process_result.stdout.splitlines():
            if line.startswith("Total Events:"):
                event_count = int(line.split(":")[1].strip())
                break
    finally:
        stop_eventjournaler()
        shutil.rmtree(detections_dir)

        if os.path.exists(tmp_detections_dir):
            shutil.move(tmp_detections_dir, detections_dir)

        start_eventjournaler()

    result = {"start_time": start_time,
              "end_time": end_time,
              "duration": end_time - start_time,
              "number_of_events_sent": int(number_of_events_to_send),
              "event_count": event_count,
              "success": event_count > 1}

    return result


def print_usage_and_exit(script_name):
    print(f"Usage: {script_name} <NumberOfEvents>")
    print(f"Example: {script_name} 1000 ")
    exit()


def main(args):
    number_of_args = len(args) - 1

    if number_of_args == 1:
        if args[1] == "--help" or args[1] == "-h":
            print_usage_and_exit(args[0])

    if number_of_args != 1:
        print_usage_and_exit(args[0])

    number_of_events_to_send = args[1]
    result = run_ingestion_test(number_of_events_to_send)
    print(f"RESULTS:{json.dumps(result)}")
    return result["success"]


if __name__ == '__main__':
    main(sys.argv)
