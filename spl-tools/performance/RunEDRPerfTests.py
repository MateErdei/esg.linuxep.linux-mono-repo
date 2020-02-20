import logging
import requests
import sys
import time
import socket
import argparse
import subprocess
import os


def add_options():
    parser = argparse.ArgumentParser(description='Performance test runner for EDR')
    parser.add_argument('-s', '--suite', default='gcc', action='store',
                        choices=['gcc', 'local-livequery', 'central-livequery'],
                        help="Select which performance test suite to run")
    parser.add_argument('-e', '--email', default='darwinperformance@sophos.xmas.testqa.com', action='store',
                        help="Central account email address to use to run live queries")
    parser.add_argument('-p', '--password', action='store', help="Central account password to use to run live queries")
    return parser


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
              "eventname": event_name, "start": str(start_time), "finish": str(end_time), "duration": str(duration)}

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
    # print("Running GCC performance test")
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
        ("all-processes", "SELECT * FROM processes;", 3),
        ("all-users", "SELECT * FROM users;", 5)
    ]

    for name, query, times_to_run in queries_to_run:
        date_time = get_current_date_time_string()
        start_time = get_current_unix_epoch_in_seconds()
        command = ['python3', local_live_query_script, name, query, str(times_to_run)]
        process_result = subprocess.run(command, timeout=120)
        if process_result.returncode != 0:
            logging.error("Running local live query failed. return code: {}, stdout: {}, stderr: {}".format(
                process_result.returncode, process_result.stdout, process_result.stderr))
            continue
        end_time = get_current_unix_epoch_in_seconds()
        record_result("local-query_{}_x{}".format(name, str(times_to_run)), date_time, start_time, end_time)


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
        ("all-processes", "SELECT * FROM processes;"),
        ("all-users", "SELECT * FROM users;")
    ]

    for name, query in queries_to_run:
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
        record_result("central-live-query_{}".format(name), date_time, start_time, end_time)


def main():
    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    parser = add_options()
    args = parser.parse_args()

    logging.info("Starting tests")

    if args.suite == 'gcc':
        run_gcc_perf_test()
    if args.suite == 'local-livequery':
        run_local_live_query_perf_test()
    if args.suite == 'central-livequery':
        run_central_live_query_perf_test(args.email, args.password)


if __name__ == '__main__':
    sys.exit(main())
