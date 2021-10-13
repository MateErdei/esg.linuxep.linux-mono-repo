#!/usr/bin/env python3
import argparse
import json
import os
import shutil
import subprocess
import sys
import time

from PerformanceResources import stop_sspl_process, start_sspl_process, get_current_unix_epoch_in_seconds


def run_ingestion_test(number_of_events_to_send, timeout='120', sleep='1000', expect_to_timeout=False):
    start_time = 0
    end_time = 0

    journal_dir = "/opt/sophos-spl/plugins/eventjournaler/data/eventjournals"
    detections_dir = f"{journal_dir}/SophosSPL/Detections"
    tmp_detections_dir = "/tmp/Detections"

    shutil.rmtree(tmp_detections_dir, ignore_errors=True)

    if os.path.exists(detections_dir):
        stop_sspl_process('eventjournaler')
        shutil.move(detections_dir, tmp_detections_dir)
        start_sspl_process('eventjournaler')
        # Allow event journaler some time to start.
        time.sleep(5)

    try:
        print(f"Sending {number_of_events_to_send} events")

        start_time = get_current_unix_epoch_in_seconds()
        command = ['/root/performance/EventPubSub',
                   '-s', "/opt/sophos-spl/var/ipc/events.ipc",
                   '-c', number_of_events_to_send,
                   '-z', sleep,
                   'send']

        try:
            process_result = subprocess.run(command, timeout=int(timeout), stdout=subprocess.PIPE, encoding="utf-8")

            if expect_to_timeout:
                raise "Expected to timeout"
        except subprocess.TimeoutExpired as ex:
            if not expect_to_timeout:
                raise "Unexpected test timeout"

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
        stop_sspl_process('eventjournaler')
        shutil.rmtree(detections_dir)
        if os.path.exists(tmp_detections_dir):
            shutil.move(tmp_detections_dir, detections_dir)
        start_sspl_process('eventjournaler')

    result = {"start_time": start_time,
              "end_time": end_time,
              "duration": end_time - start_time,
              "number_of_events_sent": int(number_of_events_to_send),
              "event_count": event_count,
              "success": event_count > 1}

    return result


def add_options():
    parser = argparse.ArgumentParser(description='Runs Event Journaler Ingestion Tests')
    parser.add_argument('-n', '--number_of_events_to_send', action='store', help="Number of events to send")
    parser.add_argument('-t', '--timeout', action='store', help="Timeout value for EventPubSub tool")
    parser.add_argument('-s', '--sleep', action='store', help="Sleep value for EventPubSub tool")
    parser.add_argument('-e', '--expect_to_timeout', action='store_true', default=False,
                        help="Set whether test is expected to timeout")
    return parser


def main():
    parser = add_options()
    args = parser.parse_args()

    result = run_ingestion_test(args.number_of_events_to_send, args.timeout, args.sleep, args.expect_to_timeout)
    print(f"RESULTS:{json.dumps(result)}")

    if result["success"]:
        return 0

    return 1


if __name__ == '__main__':
    sys.exit(main())
