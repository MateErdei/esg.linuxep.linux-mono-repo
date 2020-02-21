import logging
import requests
import sys
import time
import socket
import argparse
import subprocess
import os

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

PACKAGES_QUERY = ("packages", '''SELECT name, version, arch, revision FROM deb_packages;''')

HOSTNAME_QUERY = ("hostname", '''SELECT system_info.hostname, system_info.local_hostname FROM system_info;''')

ALL_PROCESSES_QUERY = ("all-processes", "SELECT * FROM processes;")

ALL_USERS_QUERY = ("all-users", "SELECT * FROM users;")


def get_part_after_equals(key_value_pair):
    parts = key_value_pair.strip().split(" = ")
    if len(parts) > 1:
        return parts[1]
    return ""


def record_result(event_name, date_time, start_time, end_time):
    hostname = socket.gethostname()
    build_date = "unknown"
    product_version = "unknown"
    with open("/opt/sophos-spl/base/VERSION.ini", 'r') as version_file:
        for line in version_file:
            if "BUILD_DATE" in line:
                build_date = get_part_after_equals(line)
            if "PRODUCT_VERSION" in line:
                product_version = get_part_after_equals(line)

    duration = end_time - start_time

    result = {"datetime": date_time, "hostname": hostname, "build_date": build_date, "product_version": product_version,
              "eventname": event_name, "start": start_time, "finish": end_time, "duration": str(duration)}

    r = requests.post('http://sspl-perf-mon:9200/perf-custom/_doc', json=result)
    if r.status_code not in [200, 201]:
        logging.error("Failed to store test result: {}".format(str(result)))
        logging.error("Status code: {}, text: {}".format(r.status_code, r.text))
    else:
        logging.info("Stored result for: {}".format(event_name))


def get_current_date_time_string():
    return time.strftime("%Y/%m/%d %H:%M:%S")


def get_current_unix_epoch_in_seconds():
    return int(time.time())


def run_gcc_perf_test():
    logging.info("Running GCC performance test")
    this_dir = os.path.dirname(os.path.realpath(__file__))
    build_gcc_script = os.path.join(this_dir, "build-gcc-only.sh")

    date_time = get_current_date_time_string()
    start_time = get_current_unix_epoch_in_seconds()
    subprocess.run(['bash', build_gcc_script], timeout=10000)
    end_time = get_current_unix_epoch_in_seconds()

    record_result("gcc", date_time, start_time, end_time)


def run_local_live_query_perf_test():
    logging.info("Running Local Live Query performance test")
    this_dir = os.path.dirname(os.path.realpath(__file__))
    local_live_query_script = os.path.join(this_dir, "RunLocalLiveQuery.py")

    queries_to_run = [
        (ALL_PROCESSES_QUERY, 1),
        (ALL_USERS_QUERY, 1),
        (HOSTNAME_QUERY, 1),
        (ALL_PROCESSES_QUERY, 10),
        (ALL_USERS_QUERY, 10),
        (HOSTNAME_QUERY, 10),
        (ALL_PROCESSES_QUERY, 100),
        (ALL_USERS_QUERY, 100),
        (HOSTNAME_QUERY, 100)
    ]

    for (name, query), times_to_run in queries_to_run:
        date_time = get_current_date_time_string()
        start_time = get_current_unix_epoch_in_seconds()
        command = ['python3', local_live_query_script, name, query, str(times_to_run)]
        process_result = subprocess.run(command, timeout=120)
        if process_result.returncode != 0:
            logging.error("Running local live query failed. return code: {}, stdout: {}, stderr: {}".format(
                process_result.returncode, process_result.stdout, process_result.stderr))
            continue
        end_time = get_current_unix_epoch_in_seconds()
        event_name = "local-query_{}_x{}".format(name, str(times_to_run))
        record_result(event_name, date_time, start_time, end_time)


def run_central_live_query_perf_test(email, password):
    logging.info("Running Local Live Query performance test")
    this_dir = os.path.dirname(os.path.realpath(__file__))
    central_live_query_script = os.path.join(this_dir, "cloudClient.py")

    # This machine - this script is intended to run on the test machine.
    target_machine = socket.gethostname()

    # Make sure we're logged in, no point timing that.
    subprocess.run(
        ['python3', central_live_query_script, '--region', 'q', '--email', email, '--password', password, 'login'],
        timeout=120)

    queries_to_run = [
        ALL_PROCESSES_QUERY,
        ALL_USERS_QUERY,
        HOSTNAME_QUERY
    ]

    for (name, query) in queries_to_run:
        date_time = get_current_date_time_string()
        start_time = get_current_unix_epoch_in_seconds()
        command = ['python3', central_live_query_script, '--region', 'q', '--email', email, '--password', password,
                   'run_live_query_and_wait_for_response', name, query, target_machine]
        process_result = subprocess.run(command, timeout=120)
        if process_result.returncode != 0:
            logging.error("Running live query through central failed. return code: {}, stdout: {}, stderr: {}".format(
                process_result.returncode, process_result.stdout, process_result.stderr))
            continue
        end_time = get_current_unix_epoch_in_seconds()
        event_name = "central-live-query_{}".format(name)
        record_result(event_name, date_time, start_time, end_time)


def add_options():
    parser = argparse.ArgumentParser(description='Performance test runner for EDR')
    parser.add_argument('-s', '--suite', action='store', choices=['gcc', 'local-livequery', 'central-livequery'],
                        help="Select which performance test suite to run")
    parser.add_argument('-e', '--email', default='darwinperformance@sophos.xmas.testqa.com', action='store',
                        help="Central account email address to use to run live queries")
    parser.add_argument('-p', '--password', action='store', help="Central account password to use to run live queries")
    return parser


def main():
    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    parser = add_options()
    args = parser.parse_args()

    logging.info("Starting")

    if args.suite == 'gcc':
        run_gcc_perf_test()
    if args.suite == 'local-livequery':
        run_local_live_query_perf_test()
    if args.suite == 'central-livequery':
        run_central_live_query_perf_test(args.email, args.password)

    logging.info("Finished")


if __name__ == '__main__':
    sys.exit(main())
