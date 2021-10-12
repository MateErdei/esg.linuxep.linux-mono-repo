import argparse
import json
import logging
import os
import shutil
import socket
import subprocess
import sys
import time

import requests

from performance.PerformanceResources import stop_sspl_process, start_sspl_process, get_current_unix_epoch_in_seconds, \
    wait_for_plugin_to_be_installed

PROCESS_EVENTS_QUERY = ("process-events", '''SELECT
GROUP_CONCAT(process_events.pid) AS pids,
REPLACE(process_events.path,
        (SELECT REGEX_SPLIT(process_events.path, "[\.\w-]+$", 0)), "" ) AS name,
SUBSTR(process_events.cmdline, 1, 32766) AS cmdline,
GROUP_CONCAT(process_events.parent) AS parents,
process_events.path,
process_events.gid,
process_events.uid,
process_events.euid,
process_events.egid,
hash.sha1,
hash.sha256
FROM process_events
JOIN hash AS hash
WHERE hash.path = process_events.path
GROUP BY process_events.cmdline, hash.sha1;''')

USER_EVENTS_QUERY = ("user-events",
                     '''SELECT uid, pid, message, type, path, address, terminal, time 
                        FROM user_events WHERE terminal != "cron" AND pid != 1;''')

PORTS_QUERY = ("ports",
               '''SELECT DISTINCT processes.name, listening_ports.address, listening_ports.port, processes.pid, processes.path 
                  FROM listening_ports JOIN processes USING (pid) WHERE listening_ports.address NOT LIKE '127%' AND listening_ports.protocol = 6;''')

INTERFACES_QUERY = ("interfaces",
                    '''SELECT interface_details.mtu, interface_details.interface, interface_details.mac, interface_addresses.mask, interface_addresses.address, interface_addresses.broadcast, interface_details.ibytes, interface_details.obytes 
                       FROM interface_addresses JOIN interface_details ON interface_addresses.interface = interface_details.interface;''')

PACKAGES_QUERY = ("packages", "SELECT name, version, arch, revision FROM deb_packages;")

HOSTNAME_QUERY = ("hostname", "SELECT system_info.hostname, system_info.local_hostname FROM system_info;")

DETECTIONS_QUERY = ("detections", "SELECT * FROM sophos_detections_journal;")

# This query will span the two sample detection event journal files: /mnt/filer6/linux/SSPL/performance/Detections
DETECTIONS_QUERY_FOR_SAMPLE_DATA = ("detections", "SELECT * FROM sophos_detections_journal where time > 1632144511;")

ALL_PROCESSES_QUERY = ("all-processes", "SELECT * FROM processes;")

ALL_USERS_QUERY = ("all-users", "SELECT * FROM users;")


def get_part_after_equals(key_value_pair):
    parts = key_value_pair.strip().split(" = ")
    if len(parts) > 1:
        return parts[1]
    return ""


def get_build_date_and_version(path):
    build_date = None
    product_version = None
    try:
        with open(path, 'r') as version_file:
            for line in version_file:
                if "BUILD_DATE" in line:
                    build_date = get_part_after_equals(line)
                if "PRODUCT_VERSION" in line:
                    product_version = get_part_after_equals(line)
    except Exception as ex:
        logging.warning(f"Could not read version file for: {path}")
        logging.warning(ex)
        pass
    return build_date, product_version


# Expects start_time and end_time as a float of seconds unix epochs
def record_result(event_name, date_time, start_time, end_time, custom_data=None):
    hostname = socket.gethostname()
    base_build_date, base_product_version = get_build_date_and_version("/opt/sophos-spl/base/VERSION.ini")
    edr_build_date, edr_product_version = get_build_date_and_version("/opt/sophos-spl/plugins/edr/VERSION.ini")
    mtr_build_date, mtr_product_version = get_build_date_and_version("/opt/sophos-spl/plugins/mtr/VERSION.ini")

    duration = end_time - start_time

    result = {
        "datetime": date_time,
        "hostname": hostname,
        "build_date": base_build_date,
        "product_version": base_product_version,
        "eventname": event_name,
        "start": int(start_time),
        "finish": int(end_time),
        "duration": duration}

    if custom_data:
        result.update(custom_data)

    if edr_product_version and edr_build_date:
        result["edr_product_version"] = edr_product_version
        result["edr_build_date"] = edr_build_date

    if mtr_product_version and mtr_build_date:
        result["mtr_product_version"] = mtr_product_version
        result["mtr_build_date"] = mtr_build_date

    if dry_run:
        print(result)
    else:
        r = requests.post('http://sspl-perf-mon:9200/perf-custom/_doc', json=result)
        if r.status_code not in [200, 201]:
            logging.error(f"Failed to store test result: {str(result)}")
            logging.error(f"Status code: {r.status_code}, text: {r.text}")
        else:
            logging.info(f"Stored result for: {event_name}")
            logging.info(f"Content: {result}")


# This is UTC
def get_current_date_time_string():
    return time.strftime("%Y/%m/%d %H:%M:%S", time.gmtime())


def run_gcc_perf_test():
    logging.info("Running GCC performance test")
    this_dir = os.path.dirname(os.path.realpath(__file__))
    build_gcc_script = os.path.join(this_dir, "build-gcc-only.sh")

    date_time = get_current_date_time_string()
    start_time = get_current_unix_epoch_in_seconds()
    subprocess.run(['bash', build_gcc_script], timeout=10000)
    end_time = get_current_unix_epoch_in_seconds()

    record_result("GCC Build", date_time, start_time, end_time)


def run_local_live_query_perf_test():
    logging.info("Running Local Live Query performance test")
    this_dir = os.path.dirname(os.path.realpath(__file__))
    local_live_query_script = os.path.join(this_dir, "RunLocalLiveQuery.py")

    # Queries to run and the number of times to run them, each batch will be timed.
    queries_to_run = [
        (ALL_PROCESSES_QUERY, 1),
        (ALL_USERS_QUERY, 1),
        (HOSTNAME_QUERY, 1),
        (ALL_PROCESSES_QUERY, 10),
        (ALL_USERS_QUERY, 10),
        (HOSTNAME_QUERY, 10),
        (ALL_PROCESSES_QUERY, 30),
        (ALL_USERS_QUERY, 30),
        (HOSTNAME_QUERY, 30)
    ]

    for (name, query), times_to_run in queries_to_run:
        date_time = get_current_date_time_string()
        command = ['python3', local_live_query_script, name, query, str(times_to_run)]
        logging.info(f"Running command:{str(command)}")
        process_result = subprocess.run(command, timeout=120, stdout=subprocess.PIPE, encoding="utf-8")
        if process_result.returncode != 0:
            logging.error(f"Running local live query failed. return code: {process_result.returncode}, "
                          f"stdout: {process_result.stdout}, stderr: {process_result.stderr}")
            continue

        result = json.loads(process_result.stdout)
        event_name = f"local-query_{name}_x{str(times_to_run)}"
        record_result(event_name, date_time, result["start_time"], result["end_time"])


def run_local_live_query_detections_perf_test():
    logging.info("Running Local Live Query Detections performance test")
    this_dir = os.path.dirname(os.path.realpath(__file__))
    local_live_query_script = os.path.join(this_dir, "RunLocalLiveQuery.py")

    detections_dir = "/opt/sophos-spl/plugins/eventjournaler/data/eventjournals/SophosSPL/Detections"
    sample_detections_dir = "/root/performance/Detections"
    tmp_detections_dir = "/tmp/Detections"

    # Queries to run and the number of times to run them, each batch will be timed.
    queries_to_run = [
        (DETECTIONS_QUERY_FOR_SAMPLE_DATA, 1),
        (DETECTIONS_QUERY_FOR_SAMPLE_DATA, 10)
    ]

    if not os.path.exists(detections_dir):
        os.makedirs(detections_dir)

    stop_sspl_process('eventjournaler')
    shutil.move(detections_dir, tmp_detections_dir)
    shutil.copytree(sample_detections_dir, detections_dir)

    start_sspl_process('eventjournaler')
    # Allow event journaler some time to start.
    time.sleep(5)

    try:
        for (name, query), times_to_run in queries_to_run:
            date_time = get_current_date_time_string()
            command = ['python3', local_live_query_script, name, query, str(times_to_run)]
            process_result = subprocess.run(command, timeout=120, stdout=subprocess.PIPE, encoding="utf-8")
            if process_result.returncode != 0:
                logging.error(f"Running local live query detections failed. return code: {process_result.returncode}, "
                              f"stdout: {process_result.stdout}, stderr: {process_result.stderr}")
                continue

            result = json.loads(process_result.stdout)
            event_name = f"local-query_{name}_x{str(times_to_run)}"
            record_result(event_name, date_time, result["start_time"], result["end_time"])
    finally:
        stop_sspl_process('eventjournaler')
        shutil.rmtree(detections_dir)
        shutil.move(tmp_detections_dir, detections_dir)

        start_sspl_process('eventjournaler')


def run_central_live_query_perf_test(client_id, email, password, region):
    if client_id is None and email is None:
        logging.error("Please enter API client ID or email, use -h for help.")
        return
    if not password:
        logging.error("Please enter password, use -h for help.")
        return

    wait_for_plugin_to_be_installed('edr')
    logging.info("Running Local Live Query performance test")
    this_dir = os.path.dirname(os.path.realpath(__file__))
    central_live_query_script = os.path.join(this_dir, "RunCentralLiveQuery.py")

    # This machine - this script is intended to run on the test machine.
    target_machine = socket.gethostname()

    queries_to_run = [
        ALL_PROCESSES_QUERY,
        ALL_USERS_QUERY,
        HOSTNAME_QUERY,
        DETECTIONS_QUERY
    ]

    for (name, query) in queries_to_run:
        date_time = get_current_date_time_string()
        if client_id is None:
            command = ['python3', central_live_query_script,
                       '--region', region,
                       '--email', email,
                       '--password', password,
                       '--name', name,
                       '--query', query,
                       '--machine', target_machine]
        else:
            command = ['python3', central_live_query_script,
                       '--region', region,
                       '--client-id', client_id,
                       '--password', password,
                       '--name', name,
                       '--query', query,
                       '--machine', target_machine]
        process_result = subprocess.run(command, timeout=120, stdout=subprocess.PIPE, encoding="utf-8")
        if process_result.returncode != 0:
            logging.error(f"Running live query through central failed. return code: {process_result.returncode}, "
                          f"stdout: {process_result.stdout}, stderr: {process_result.stderr}")
            continue

        result = json.loads(process_result.stdout)
        event_name = f"central-live-query_{name}"
        record_result(event_name, date_time, result["start_time"], result["end_time"])


def run_local_live_response_test(number_of_terminals: int, keep_alive: int):
    logging.info("Running local Live Response terminal performance test")
    wait_for_plugin_to_be_installed('liveresponse')
    this_dir = os.path.dirname(os.path.realpath(__file__))
    local_live_terminal_script = os.path.join(this_dir, "RunLocalLiveTerminal.py")
    message_contents_file_path = os.path.join(this_dir, "1000Chars")
    date_time = get_current_date_time_string()
    command = ['python3.7', local_live_terminal_script,
               "-f", message_contents_file_path,
               "-n", str(number_of_terminals),
               "-k", str(keep_alive)]
    logging.info(f"Running command:{str(command)}")
    process_result = subprocess.run(command, timeout=500, stdout=subprocess.PIPE, encoding="utf-8")
    if process_result.returncode != 0:
        logging.error(f"Running local live response terminal failed. return code: {process_result.returncode}, "
                      f"stdout: {process_result.stdout}, stderr: {process_result.stderr}")

    # We get lots of logging to stdout from the ltserver, we need to identify the results only. Example results line:
    # RESULTS:{"start_time": 1595956922.316, "end_time": 1595956922.328, "duration": 0.0111, "success": true}
    result = None
    results_tag = "RESULTS:"
    for line in process_result.stdout.splitlines():
        if line.startswith(results_tag):
            result = json.loads(line.lstrip(results_tag))
            break

    if not result:
        logging.error("No result from RunLocalLiveTerminal.py")
        return

    # Have to handle the two variety of tests here:
    # 1) Test plan wants a session to remain open for 5 mins to check for resource usage during that time
    # 2) Test plan also wants to time how long a number of characters take to be sent
    # The way these results are processed means that we need a result start and end to cover the 5 mins we want
    # resource stats for and we need a separate result with a start and end for sending of X characters test.
    # Decided to do two separate tests to cover this instead of trying to get a single test to cover both requirements
    # and then having to add a fake result in. Event name examples:
    # local-liveresponse_x1 -> one terminal receiving a string and then closing, not held open for any extra time.
    # local-liveresponse_x10 -> ten terminals receiving a string and then closing, not held open for any extra time.
    # local-liveresponse-keepalive_x1 -> one terminal receiving a string and then being held open for e.g. 5 mins.
    # local-liveresponse-keepalive_x10 -> ten terminals receiving a string and then being held open for e.g. 5 mins.
    event_name = f"local-liveresponse_x{number_of_terminals}"
    if keep_alive != 0:
        event_name = f"local-liveresponse-keepalive_x{number_of_terminals}"

    record_result(event_name, date_time, result["start_time"], result["end_time"])


def run_event_journaler_ingestion_test():
    logging.info("Running Event Journaler Ingestion Test")

    this_dir = os.path.dirname(os.path.realpath(__file__))
    event_journaler_ingestion_script = os.path.join(this_dir, "RunEventJournalerIngestionTest.py")

    date_time = get_current_date_time_string()
    command = ['python3', event_journaler_ingestion_script, '220']
    logging.info(f"Running command:{str(command)}")
    process_result = subprocess.run(command, timeout=500, stdout=subprocess.PIPE, encoding="utf-8")
    if process_result.returncode != 0:
        logging.error(f"Running event journaler ingestion test failed. return code: {process_result.returncode}, "
                      f"stdout: {process_result.stdout}, stderr: {process_result.stderr}")

    result = None
    results_tag = "RESULTS:"
    for line in process_result.stdout.splitlines():
        if line.startswith(results_tag):
            result = json.loads(line.lstrip(results_tag))
            break

    if not result:
        logging.error("No result from RunEventJournalerIngestionTest.py")
        return

    print(result)
    custom_data = {"number_of_events_sent": result["number_of_events_sent"], "event_count": result["event_count"]}
    record_result("event-journaler-ingestion", date_time, result["start_time"], result["end_time"], custom_data=custom_data)


def add_options():
    parser = argparse.ArgumentParser(description='Performance test runner for EDR')

    parser.add_argument('-s', '--suite', action='store',
                        choices=['gcc',
                                 'local-livequery',
                                 'local-livequery-detections',
                                 'central-livequery',
                                 'local-liveresponse_x1',
                                 'local-liveresponse_x10',
                                 'event-journaler-ingestion'],
                        help="Select which performance test suite to run")

    parser.add_argument('-i', '--client-id', action='store',
                        help="Central account API client ID to use to run live queries")

    parser.add_argument('-e', '--email', default='darwinperformance@sophos.xmas.testqa.com', action='store',
                        help="Central account email address to use to run live queries")

    parser.add_argument('-p', '--password', action='store',
                        help="Central account API client secret or password to use to run live queries")

    parser.add_argument('-r', '--region', action='store', help="Central region (q, d, p)")

    parser.add_argument('-d', '--dry_run', action='store', chioces=[True, False],
                        help="Run tests locally without storing results")
    return parser


def main():
    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    parser = add_options()
    args = parser.parse_args()

    global dry_run
    dry_run = args.dry_run

    logging.info("Starting")

    if args.suite == 'gcc':
        run_gcc_perf_test()
    elif args.suite == 'local-livequery':
        run_local_live_query_perf_test()
    elif args.suite == 'local-livequery-detections':
        run_local_live_query_detections_perf_test()
    elif args.suite == 'central-livequery':
        run_central_live_query_perf_test(args.client_id, args.email, args.password, args.region)
    elif args.suite == 'local-liveresponse_x1':
        run_local_live_response_test(1, 0)
        run_local_live_response_test(1, 300)
    elif args.suite == 'local-liveresponse_x10':
        run_local_live_response_test(10, 0)
    elif args.suite == 'event-journaler-ingestion':
        run_event_journaler_ingestion_test()

    logging.info("Finished")


if __name__ == '__main__':
    sys.exit(main())
