import argparse
import csv
import glob
import hashlib
import io
import json
import os.path
import re
import socket
import stat
import statistics
import sys
import tarfile
import xml.etree.ElementTree
import zipfile

from artifactory import ArtifactoryPath

from PerformanceResources import *
from RunResponseActions import *
from Annotations import annotate_graphs, add_annotation, delete_annotations, patch_onaccess_performance_test_annotation, add_onaccess_performance_test_annotation

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
SAFESTORE_MALWARE_PATH = "/root/performance/malware_for_safestore_tests"

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


def fetch_artifacts(project, repo, artifact_path, output_dir=SCRIPT_DIR):
    if not os.path.isdir(output_dir):
        os.makedirs(output_dir)

    builds = []
    repo_url = f"https://artifactory.sophos-ops.com/artifactory/esg-build-tested/{project}.{repo}/develop"

    for build in ArtifactoryPath(repo_url):
        builds.append(build)
    latest_build = sorted(builds, key=lambda x: int(os.path.basename(x).split('-')[0]), reverse=True)[0]

    artifact_url = os.path.join(latest_build, "build", f"{artifact_path}.zip")

    r = requests.get(artifact_url)
    z = zipfile.ZipFile(io.BytesIO(r.content))
    z.extractall(output_dir)

    logging.info(f"Contents of working directory after fetching artifact: {os.listdir(SCRIPT_DIR)}")


def get_test_inputs_from_base():
    fetch_artifacts("linuxep", "linux-mono-repo", "base/linux_x64_rel/SystemProductTestOutput")

    if not os.path.exists(os.path.join(SCRIPT_DIR, "cloudClient.py")):
        logging.error(f"cloudClient.py does not exist: {os.listdir(SCRIPT_DIR)}")
        exit(Jenkins_Job_Return_Code.FAILURE)

    if not os.path.exists(os.path.join(SCRIPT_DIR, "SophosHTTPSClient.py")):
        logging.error(f"SophosHTTPSClient.py does not exist: {os.listdir(SCRIPT_DIR)}")
        exit(Jenkins_Job_Return_Code.FAILURE)

    os.environ["CLOUD_CLIENT_SCRIPT"] = os.path.join(SCRIPT_DIR, "cloudClient.py")


def get_test_inputs_from_event_journaler():
    fetch_artifacts("linuxep", "linux-mono-repo", "eventjournaler/linux_x64_rel/manualTools")

    event_pub_sub_tool_path = os.path.join(SCRIPT_DIR, "EventPubSub")
    journal_reader_tool_path = os.path.join(SCRIPT_DIR, "JournalReader")

    if not os.path.exists(event_pub_sub_tool_path):
        logging.error(f"EventPubSub does not exist: {os.listdir(SCRIPT_DIR)}")
        exit(Jenkins_Job_Return_Code.FAILURE)

    if not os.path.exists(journal_reader_tool_path):
        logging.error(f"JournalReader does not exist: {os.listdir(SCRIPT_DIR)}")
        exit(Jenkins_Job_Return_Code.FAILURE)

    os.chmod(event_pub_sub_tool_path, os.stat(event_pub_sub_tool_path).st_mode | stat.S_IEXEC)
    os.chmod(journal_reader_tool_path, os.stat(journal_reader_tool_path).st_mode | stat.S_IEXEC)

    os.environ["EVENT_PUB_SUB"] = event_pub_sub_tool_path
    os.environ["JOURNAL_READER"] = journal_reader_tool_path


def get_safestore_tool():
    fetch_artifacts("core", "safestore", "release/linux-x64/safestore")
    safestore_tool = os.path.join(SCRIPT_DIR, "ssr", "ssr")

    os.chmod(safestore_tool, os.stat(safestore_tool).st_mode | stat.S_IEXEC)
    os.environ["SAFESTORE_TOOL"] = safestore_tool


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

    if dry_run:
        logging.info(result)
    else:
        r = requests.post('http://sspl-perf-mon:9200/perf-custom/_doc', json=result)
        if r.status_code not in [200, 201]:
            logging.error(f"Failed to store test result: {str(result)}")
            logging.error(f"Status code: {r.status_code}, text: {r.text}")
            exit(Jenkins_Job_Return_Code.FAILURE)
        else:
            logging.info(f"Stored result for: {event_name}")
            logging.info(f"Content: {result}")


# This is UTC
def get_current_date_time_string():
    return time.strftime("%Y/%m/%d %H:%M:%S", time.gmtime())


def run_gcc_perf_test():
    logging.info("Running GCC performance test")
    build_gcc_script = os.path.join(SCRIPT_DIR, "build-gcc-only.sh")

    date_time = get_current_date_time_string()
    start_time = get_current_unix_epoch_in_seconds()
    result = subprocess.run(['bash', build_gcc_script], timeout=10000)
    end_time = get_current_unix_epoch_in_seconds()

    record_result("GCC Build", date_time, start_time, end_time)
    add_annotation(tag="gcc-build", start_time=int(start_time * 1000), end_time=int(end_time * 1000), text="Building GCC")

    if result.returncode != 0:
        exit(Jenkins_Job_Return_Code.FAILURE)


def enable_perf_dump():
    log_utils = LogUtils.LogUtils()
    subprocess.run(["/opt/sophos-spl/bin/wdctl", "stop", "on_access_process"])
    oa_product_config_contents = "{\"dumpPerfData\": true}"
    with open("/opt/sophos-spl/plugins/av/var/onaccessproductconfig.json", "w") as f:
        f.write(oa_product_config_contents)
    mark = log_utils.get_on_access_log_mark()
    subprocess.run(["/opt/sophos-spl/bin/wdctl", "start", "on_access_process"])
    log_utils.wait_for_on_access_log_contains_after_mark("On-access scanning enabled", mark)
    detect_eicar()


def disable_perf_dump():
    subprocess.run(["/opt/sophos-spl/bin/wdctl", "stop", "on_access_process"])
    os.remove("/opt/sophos-spl/plugins/av/var/onaccessproductconfig.json")
    subprocess.run(["/opt/sophos-spl/bin/wdctl", "start", "on_access_process"])


def detect_eicar():
    log_utils = LogUtils.LogUtils()
    mark = log_utils.get_on_access_log_mark()
    f = open("/tmp/eicar.com", "w")
    f.write("X5O!P%@AP[4\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*")
    f.close()
    log_utils.wait_for_on_access_log_contains_after_mark(
        "OnAccessImpl <> Detected \"/tmp/eicar.com\" is infected with EICAR-AV-Test", mark)


def run_clean_file_test(test_name, stop_on_queue_full, max_count, oa_enabled=False):
    if oa_enabled:
        enable_perf_dump()
    dirpath = os.path.join("/tmp", "onaccess_stress_test")
    os.makedirs(dirpath)
    date_time = get_current_date_time_string()

    log_utils = LogUtils.LogUtils()
    mark = log_utils.get_on_access_log_mark()
    start_time = get_current_unix_epoch_in_seconds()
    file_count = 0
    while file_count < max_count:
        file_count += 1
        filepath = os.path.join(dirpath, f"{file_count}.txt")
        with open(filepath, "w") as f:
            f.write(f"clean file file_count{file_count}")
        if stop_on_queue_full:
            try:
                log_utils.check_on_access_log_does_not_contain_after_mark(
                    "Failed to add scan request to queue, on-access scanning queue is full.", mark)
            except AssertionError as ex:
                logging.info(f"At file count {file_count}: {str(ex)}")
                break
    end_time = get_current_unix_epoch_in_seconds()
    logging.info(f"Created {file_count} files in {end_time - start_time} seconds")

    custom_data = {"file_count": file_count}
    if oa_enabled:
        custom_data.update(get_product_results())
        disable_perf_dump()
        for perfData in glob.glob("/opt/sophos-spl/plugins/av/var/perfDumpThread*"):
            os.remove(perfData)
    shutil.rmtree(dirpath)
    record_result(test_name, date_time, start_time, end_time, custom_data=custom_data)
    return file_count


def run_onaccess_test(max_file_count):
    logging.info("Running AV On-access stress test")
    log_utils = LogUtils.LogUtils()
    oa_mark = log_utils.get_on_access_log_mark()
    td_mark = log_utils.get_sophos_threat_detector_log_mark()
    wd_mark = log_utils.get_wdctl_log_mark()

    # Write clean files until the queue becomes full or we reach max_count
    file_count = run_clean_file_test("File opens - OA enabled", True, max_file_count, True)
    time.sleep(60)

    # Write the same number of files but with on-access disable
    disable_onaccess()
    run_clean_file_test("File opens - OA disabled", False, file_count)
    enable_onaccess()
    time.sleep(60)

    # Write the same number of files but with the product not running
    stop_sspl()
    run_clean_file_test("File opens - SPL not running", False, file_count)
    oa_mark2 = log_utils.get_on_access_log_mark()
    start_sspl()
    time.sleep(60)

    # Check for FATAL errors
    log_utils.check_on_access_log_does_not_contain_after_mark("FATAL", oa_mark)
    log_utils.check_sophos_threat_detector_log_does_not_contain_after_mark("FATAL", td_mark)
    log_utils.check_wdctl_log_does_not_contain_after_mark("died with", wd_mark)
    log_utils.wait_for_on_access_log_contains_after_mark(
        "soapd_bootstrap <> No policy override, following policy settings", oa_mark2)

    # Check that the product still works
    detect_eicar()


def run_slow_scan_test(test_name, stop_on_queue_full, max_count, oa_enabled=False):
    if oa_enabled:
        enable_perf_dump()
    dirpath = os.path.join("/tmp", "onaccess_stress_test_slow_scan")
    os.makedirs(dirpath)
    date_time = get_current_date_time_string()

    log_utils = LogUtils.LogUtils()
    mark = log_utils.get_on_access_log_mark()
    start_time = get_current_unix_epoch_in_seconds()
    file_count = 0
    while file_count < max_count:
        file_count += 1
        destpath = os.path.join(dirpath, f"rt{file_count}.jar")
        shutil.copyfile("rt.jar", destpath)
        if stop_on_queue_full:
            try:
                log_utils.check_on_access_log_does_not_contain_after_mark(
                    "Failed to add scan request to queue, on-access scanning queue is full.", mark)
            except AssertionError as ex:
                logging.info(f"At file count {file_count}: {str(ex)}")
                break
    end_time = get_current_unix_epoch_in_seconds()
    logging.info(f"Created {file_count} files in {end_time - start_time} seconds")

    shutil.rmtree(dirpath)
    custom_data = {"file_count": file_count}
    if oa_enabled:
        custom_data.update(get_product_results())
        disable_perf_dump()
        for perfData in glob.glob("/opt/sophos-spl/plugins/av/var/perfDumpThread*"):
            os.remove(perfData)
    record_result(test_name, date_time, start_time, end_time, custom_data=custom_data)
    return file_count


def run_slow_scan_onaccess_test(max_file_count):
    logging.info("Running AV On-access stress test with slow-scanning files")
    log_utils = LogUtils.LogUtils()
    oa_mark = log_utils.get_on_access_log_mark()
    td_mark = log_utils.get_sophos_threat_detector_log_mark()
    wd_mark = log_utils.get_wdctl_log_mark()

    # Write clean files until the queue becomes full or we reach max_count
    file_count = run_slow_scan_test("Slow file opens - OA enabled", True, max_file_count, True)
    time.sleep(300)

    # Write the same number of files but with on-access disable
    disable_onaccess()
    run_slow_scan_test("Slow file opens - OA disabled", False, file_count)
    enable_onaccess()
    time.sleep(300)

    # Write the same number of files but with the product not running
    stop_sspl()
    run_slow_scan_test("Slow file opens - SPL not running", False, file_count)
    oa_mark2 = log_utils.get_on_access_log_mark()
    start_sspl()
    time.sleep(300)

    # Check for FATAL errors
    log_utils.check_on_access_log_does_not_contain_after_mark("FATAL", oa_mark)
    log_utils.check_sophos_threat_detector_log_does_not_contain_after_mark("FATAL", td_mark)
    log_utils.check_wdctl_log_does_not_contain_after_mark("died with", wd_mark)
    log_utils.wait_for_on_access_log_contains_after_mark(
        "soapd_bootstrap <> No policy override, following policy settings", oa_mark2)

    # Check that the product still works
    detect_eicar()


def get_product_results():
    dump_glob = "/opt/sophos-spl/plugins/av/var/perfDumpThread*"

    perf_dump_file_list = glob.glob(dump_glob)

    logging.info(f"Found dump: {perf_dump_file_list}")

    average_scan_time_list = []
    average_waiting_time_list = []
    average_queue_size_list = []
    total_scans = 0
    for dump in perf_dump_file_list:
        logging.info(f"Processing {dump}")

        scan_duration_results = []
        in_product_duration_results = []
        queue_size_at_point_of_insertion = []
        regex = re.compile(r"perfDumpThread(\d+)_")
        regex_result = regex.search(dump)
        thread_id = regex_result.group(1)
        with open(dump) as f:
            tsv_results = csv.DictReader(f, delimiter="\t")
            for line in tsv_results:
                scan_duration_results.append(int(line["Scan duration"]))
                in_product_duration_results.append(int(line["In-Product duration"]))
                queue_size_at_point_of_insertion.append(int(line["Queue size"]))

            if len(scan_duration_results) == 0:
                logging.info("Skipping file no scans on this thread")
                continue

            average_scan_time = round(statistics.mean(scan_duration_results) / 100, 3)
            average_waiting_time = round(statistics.mean(in_product_duration_results) / 100, 3)
            average_queue_size = round(statistics.mean(queue_size_at_point_of_insertion))
            total_scans = total_scans + len(scan_duration_results)
            logging.info(f"Scans for thread {thread_id}: {len(scan_duration_results)}")
            logging.info(f"Average scan time for thread {thread_id} is {average_scan_time}s")
            logging.info(f"Average waiting time in product for thread {thread_id} is {average_waiting_time}s")
            logging.info(f"Average queue size in product for thread {thread_id} is {average_queue_size}")

        average_scan_time_list.append(average_scan_time)
        average_waiting_time_list.append(average_waiting_time)
        average_queue_size_list.append(average_queue_size)

        logging.info("")

    average_scan_times = round(statistics.mean(average_scan_time_list), 3)
    average_time_in_product = round(statistics.mean(average_waiting_time_list), 3)
    average_queue_size = round(statistics.mean(average_queue_size_list))
    result_json = {
        "total_scans": total_scans,
        "average_scan_times": average_scan_times,
        "average_time_in_product": average_time_in_product,
        "avereage_queue_size": average_queue_size
    }

    logging.info(f"Total scans during testing: {total_scans}")
    logging.info(f"Total average scan time: {average_scan_times}s")
    logging.info(f"Total average waiting time in queue: {average_time_in_product}s")
    logging.info(f"Total average queue size: {average_queue_size}")

    return result_json


def run_local_live_query_perf_test():
    logging.info("Running Local Live Query performance test")
    failed_queries = 0
    local_live_query_script = os.path.join(SCRIPT_DIR, "RunLocalLiveQuery.py")

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
            failed_queries += 1
            continue

        result = json.loads(process_result.stdout)
        event_name = f"local-query_{name}_x{str(times_to_run)}"
        record_result(event_name, date_time, result["start_time"], result["end_time"])

    if failed_queries == len(queries_to_run):
        logging.error("Running all local live queries failed")
        exit(Jenkins_Job_Return_Code.FAILURE)
    elif failed_queries > 0:
        logging.warning("Running some local live queries failed")
        exit(Jenkins_Job_Return_Code.UNSTABLE)


def run_local_live_query_detections_perf_test():
    logging.info("Running Local Live Query Detections performance test")
    failed_queries = 0
    local_live_query_script = os.path.join(SCRIPT_DIR, "RunLocalLiveQuery.py")

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
                failed_queries += 1
                continue

            result = json.loads(process_result.stdout)
            event_name = f"local-query_{name}_x{str(times_to_run)}"
            record_result(event_name, date_time, result["start_time"], result["end_time"])
    finally:
        stop_sspl_process('eventjournaler')
        shutil.rmtree(detections_dir)
        shutil.move(tmp_detections_dir, detections_dir)

        start_sspl_process('eventjournaler')

    if failed_queries == len(queries_to_run):
        logging.error("Running all local live detection queries failed")
        exit(Jenkins_Job_Return_Code.FAILURE)
    elif failed_queries > 0:
        logging.warning("Running some local live detection queries failed")
        exit(Jenkins_Job_Return_Code.UNSTABLE)


def run_on_access_performance_test(client_id, client_secret, client_region, on_write_toggle, on_read_toggle):
    """
    Test to measure how much On-Access affects performance of system. RunOnAccessTest.py changes On-Access settings in
    Central every 8 hours (starting from midnight).
    00:00 - 08:00, On-Read is on but On-Write is off
    08:00 - 16:00, On-read is off but On-Write is on
    16:00 - 00:00, On-read is on and On-Write is on
    Reason for changing On-Access settings via Central API and not just changing local ALC policy is that a different
    test could then override the change. Changing settings via Central makes them persist
    """

    logging.info(f"Running on access test, on_write setting: {on_write_toggle}, on_read setting: {on_read_toggle}")
    on_access_test_script = os.path.join(SCRIPT_DIR, "RunOnAccessTest.py")
    command = ['python3', on_access_test_script,
               '-i', client_id,
               '-p', client_secret,
               '-r', client_region]

    if on_write_toggle:
        command.append('--on-access-write')
    else:
        command.append('--no-on-access-write')

    if on_read_toggle:
        command.append('--on-access-read')
    else:
        command.append('--no-on-access-read')

    get_test_inputs_from_base() # Getting cloudCLient.py and SophosHTTPSClient.py
    process_result = subprocess.run(command, timeout=60, stdout=subprocess.PIPE, encoding="utf-8")
    if process_result.returncode != Jenkins_Job_Return_Code.SUCCESS:
        logging.error(f"Running On Access test and trying to change settings through central failed. "
                      f"return code: {process_result.returncode}, "
                      f"stdout: {process_result.stdout}, stderr: {process_result.stderr}")
        exit(process_result.returncode)
    else:
        # Not trivial to work with annotations as the test has three states that each last 8 hours
        # Current solution is to add an annotation when on-write or on-read get turned on but not specify an end time
        # Once on-read or on-write get turned off, the annotation is patched to contain the end time
        current_time = get_current_unix_epoch_in_seconds()
        current_time_milliseconds = int(current_time * 1000)
        if on_write_toggle:
            on_write_ret = add_onaccess_performance_test_annotation(tag="on-access-write", start_time=current_time_milliseconds,
                                                                    end_time=None, text="On-Access On-Write setting toggle")
        else:
            on_write_ret = patch_onaccess_performance_test_annotation(tag="on-access-write", end_time=current_time_milliseconds)

        if on_read_toggle:
            on_read_ret = add_onaccess_performance_test_annotation(tag="on-access-read", start_time=current_time_milliseconds,
                                                                   end_time=None, text="On-Access On-Read setting toggle")
        else:
            on_read_ret = patch_onaccess_performance_test_annotation(tag="on-access-read", end_time=current_time_milliseconds)

        if on_write_ret != 0 or on_read_ret != 0:
            exit(Jenkins_Job_Return_Code.FAILURE)
        else:
            exit(Jenkins_Job_Return_Code.SUCCESS)


def run_central_live_query_perf_test(client_id, email, password, region):
    if client_id is None and email is None:
        logging.error("Please enter API client ID or email, use -h for help.")
        exit(Jenkins_Job_Return_Code.FAILURE)
    if not password:
        logging.error("Please enter password, use -h for help.")
        exit(Jenkins_Job_Return_Code.FAILURE)

    get_test_inputs_from_base()
    wait_for_plugin_to_be_installed('edr')
    logging.info("Running Central Live Query performance test")
    failed_queries = 0
    central_live_query_script = os.path.join(SCRIPT_DIR, "RunCentralLiveQuery.py")

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
        process_result = subprocess.run(command, stdout=subprocess.PIPE, encoding="utf-8")
        if process_result.returncode != 0:
            logging.error(f"Running live query through central failed. return code: {process_result.returncode}, "
                          f"stdout: {process_result.stdout}, stderr: {process_result.stderr}")
            failed_queries += 1
            continue

        result = json.loads(process_result.stdout)
        event_name = f"central-live-query_{name}"
        record_result(event_name, date_time, result["start_time"], result["end_time"])

    if failed_queries == len(queries_to_run):
        logging.error("Running all central live queries failed")
        exit(Jenkins_Job_Return_Code.FAILURE)
    elif failed_queries > 0:
        logging.warning("Running some central live queries failed")
        exit(Jenkins_Job_Return_Code.FAILURE)


def run_local_live_response_test(number_of_terminals: int, keep_alive: int):
    logging.info("Running local Live Response terminal performance test")
    fetch_artifacts("winep", "liveterminal", "websocket_server", "websocket_server")

    wait_for_plugin_to_be_installed('liveresponse')
    local_live_terminal_script = os.path.join(SCRIPT_DIR, "RunLocalLiveTerminal.py")
    message_contents_file_path = os.path.join(SCRIPT_DIR, "1000Chars")
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
        exit(Jenkins_Job_Return_Code.FAILURE)

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

    get_test_inputs_from_event_journaler()
    event_journaler_ingestion_script = os.path.join(SCRIPT_DIR, "RunEventJournalerIngestionTest.py")

    tests_to_run = [
        {'name': 'queue_saturation', 'number_of_events_to_send': '220', 'timeout': '120', 'sleep': '0',
         'expect_to_timeout': False},
        {'name': 'event_retention', 'number_of_events_to_send': '100000', 'timeout': '120', 'sleep': '500',
         'expect_to_timeout': False},
        {'name': 'queue_saturation_5min', 'number_of_events_to_send': '10000000000', 'timeout': '300', 'sleep': '0',
         'expect_to_timeout': True},
    ]

    for test_args in tests_to_run:
        date_time = get_current_date_time_string()
        command = ['python3', event_journaler_ingestion_script,
                   '-n', test_args["number_of_events_to_send"],
                   '-t', test_args["timeout"],
                   '-s', test_args["sleep"]]
        if test_args["expect_to_timeout"]:
            command.append('-e')

        logging.info(f"Running command:{str(command)}")
        process_result = subprocess.run(command, timeout=500, stdout=subprocess.PIPE, encoding="utf-8")
        if process_result.returncode != 0:
            logging.error(f"Running event journaler ingestion test ({test_args['name']}) failed. "
                          f"return code: {process_result.returncode}, stdout: {process_result.stdout}, "
                          f"stderr: {process_result.stderr}")

        result = None
        results_tag = "RESULTS:"
        for line in process_result.stdout.splitlines():
            if line.startswith(results_tag):
                result = json.loads(line.lstrip(results_tag))
                break

        if not result:
            logging.error("No result from RunEventJournalerIngestionTest.py")
            exit(Jenkins_Job_Return_Code.FAILURE)

        logging.info(result)
        event_name = f"event-journaler-ingestion_{test_args['name']}"
        custom_data = {"number_of_events_sent": result["number_of_events_sent"], "event_count": result["event_count"]}
        record_result(event_name, date_time, result["start_time"], result["end_time"], custom_data=custom_data)


def run_safestore_database_content_test():
    start_time = get_current_unix_epoch_in_seconds()

    get_safestore_tool()
    return_code = Jenkins_Job_Return_Code.SUCCESS
    quarantined_files = 0
    unquarantined_files = 0
    twenty_four_hours = 24 * 60 * 60

    safestore_db_content = [item for item in get_safestore_db_content_as_dict() if
                            item["Location"] == SAFESTORE_MALWARE_PATH]
    with open(os.path.join(SCRIPT_DIR, "expected_malware.json"), "r") as f:
        expected_malware = json.loads(f.read())

    for expected_threat in expected_malware:
        for idx, threat_info in enumerate(safestore_db_content):
            if threat_info["Name"] == expected_threat["fileName"]:
                stored_time = threat_info["Store time"]
                unix_stored_time = stored_time[stored_time.find("(") + 1:stored_time.find(")")]
                if int(unix_stored_time) < int(start_time - twenty_four_hours) or threat_info[
                    "Status"] != "quarantined":
                    logging.warning(f"Threat was not quarantined by last scheduled scan: {threat_info}")
                    unquarantined_files += 1
                    break
                else:
                    quarantined_files += 1
                    break
            elif idx == len(safestore_db_content) - 1:
                logging.warning(f"{expected_threat['fileName']} was not quarantined by last scheduled scan")
                logging.warning(f"SafeStore Database Content: {safestore_db_content}")
                unquarantined_files += 1

    if unquarantined_files == len(safestore_db_content):
        return_code = Jenkins_Job_Return_Code.FAILURE
    elif 0 < unquarantined_files:
        return_code = Jenkins_Job_Return_Code.UNSTABLE

    custom_data = {"quarantined_files": quarantined_files, "unquarantined_files": unquarantined_files}

    end_time = get_current_unix_epoch_in_seconds()
    record_result("safestore_verify_detections", get_current_date_time_string(), start_time, end_time,
                  custom_data=custom_data)
    exit(return_code)


def run_safestore_restoration_test():
    start_time = get_current_unix_epoch_in_seconds()

    log_utils = LogUtils.LogUtils()
    td_mark = log_utils.get_sophos_threat_detector_log_mark()
    av_mark = log_utils.get_av_log_mark()

    return_code, restored_files, unrestored_files = Jenkins_Job_Return_Code.SUCCESS, 0, 0
    corc_policy_path = "/opt/sophos-spl/base/mcs/policy/CORC_policy.xml"
    tmp_corc_policy_path = "/tmp/CORC_policy.xml"
    modified_policy_path = "/tmp/whitelist_CORC_policy.xml"
    policy_permissions = os.stat("/opt/sophos-spl/base/mcs/policy/CORE_policy.xml")

    try:
        stop_sspl_process("mcsrouter")
        if os.path.exists(corc_policy_path):
            shutil.copyfile(corc_policy_path, tmp_corc_policy_path)
        else:
            shutil.copyfile(os.path.join(SCRIPT_DIR, "corc_policy_empty_allowlist.xml"), tmp_corc_policy_path)

        policy = xml.etree.ElementTree.parse(tmp_corc_policy_path)
        with open(os.path.join(SCRIPT_DIR, "expected_malware.json"), "r") as f:
            expected_malware = json.loads(f.read())
        for threat in expected_malware:
            threat_item = xml.etree.ElementTree.Element("item")
            threat_item.attrib["type"] = "sha256"
            threat_item.text = threat["sha256"]
            policy.getroot().find("whitelist").append(threat_item)

        policy.write(modified_policy_path)
        os.chmod(modified_policy_path, policy_permissions.st_mode)
        os.chown(modified_policy_path, policy_permissions.st_uid, policy_permissions.st_gid)
        shutil.move(modified_policy_path, corc_policy_path)

        try:
            log_utils.wait_for_log_contains_after_mark(log_utils.sophos_threat_detector_log,
                                                       "Triggering rescan of SafeStore database",
                                                       td_mark,
                                                       120)
        except Exception as e:
            logging.warning(e)

        for threat in expected_malware:
            file_path = threat["filePath"]
            log_utils.wait_for_av_log_contains_after_mark(f"Reporting successful restoration of {file_path}", av_mark,
                                                          60)

            if os.path.exists(file_path):
                restored_files += 1
            else:
                logging.warning(f"{file_path} was not restored by SafeStore")
                unrestored_files += 1

        if unrestored_files == len(expected_malware):
            logging.error("No threats were restored from SafeStore database")
            return_code = Jenkins_Job_Return_Code.FAILURE
        elif 0 < unrestored_files:
            return_code = Jenkins_Job_Return_Code.UNSTABLE

    except Exception as e:
        logging.error(f"Failed to restore SafeStore database: {str(e)}")
        return_code = Jenkins_Job_Return_Code.FAILURE

    finally:
        if os.path.exists(tmp_corc_policy_path):
            os.chmod(tmp_corc_policy_path, policy_permissions.st_mode)
            os.chown(tmp_corc_policy_path, policy_permissions.st_uid, policy_permissions.st_gid)
            shutil.move(tmp_corc_policy_path, corc_policy_path)
        start_sspl_process("mcsrouter")

    custom_data = {"restored_files": restored_files, "unrestored_files": unrestored_files}

    end_time = get_current_unix_epoch_in_seconds()
    record_result("safestore_database_restoration", get_current_date_time_string(), start_time, end_time,
                  custom_data=custom_data)
    exit(return_code)


def run_response_actions_download_files_test(region, env, tenant_id):
    return_code = Jenkins_Job_Return_Code.SUCCESS
    failed_file_downloads = 0

    start_time = get_current_unix_epoch_in_seconds()
    log_utils = LogUtils.LogUtils()
    log_utils.mark_ra_action_runner_log()

    file_to_download = os.path.join(SCRIPT_DIR, f"file_to_download")
    create_file_for_ra(file_to_download)
    size = os.path.getsize(file_to_download)
    sha256 = hashlib.sha256(open(file_to_download, 'rb').read()).hexdigest()

    for i in range(10):
        filepath = os.path.join(SCRIPT_DIR, f"downloaded_test_file_{i}")

        res = download_response_actions_file(region=region, env=env, tenant_id=tenant_id, endpoint_id=get_endpoint_id(),
                                             file_path=filepath, size=size, sha256=sha256)
        logging.debug(res)

        upload_file_to_client_bucket(res["url"], file_to_download)

        action_status = get_response_action_status(region=region, env=env, tenant_id=tenant_id, action_id=res["id"])
        logging.debug(action_status)

        if action_status["endpoints"][0]["result"] != "succeeded":
            logging.warning(f"Failed to download response actions file {filepath}, "
                            f"due to: {action_status['endpoints'][0]['error']['errorMessage']}")
            failed_file_downloads += 1
            continue

        log_utils.wait_for_ra_action_runner_log_contains_after_mark(f"Beginning download to {filepath}", 60)
        log_utils.wait_for_ra_action_runner_log_contains_after_mark(f"{filepath} downloaded successfully", 120)

        if not os.path.exists(filepath):
            logging.warning(f"Failed to download response actions file {filepath}")
            failed_file_downloads += 1
    end_time = get_current_unix_epoch_in_seconds()

    record_result("Response Actions Download Files", get_current_date_time_string(), start_time, end_time,
                  custom_data={"failed_file_downloads": failed_file_downloads})

    if failed_file_downloads == 10:
        logging.error(f"Failed to download all response actions files")
        return_code = Jenkins_Job_Return_Code.FAILURE
    elif 0 < failed_file_downloads:
        logging.warning(f"Failed to download response actions files {failed_file_downloads} times")
        return_code = Jenkins_Job_Return_Code.UNSTABLE
    exit(return_code)


def run_response_actions_upload_files_test(region, env, tenant_id):
    return_code = Jenkins_Job_Return_Code.SUCCESS
    failed_file_uploads = 0

    start_time = get_current_unix_epoch_in_seconds()
    for i in range(10):
        filepath = os.path.join(SCRIPT_DIR, f"test_file_{i}")
        create_file_for_ra(filepath)

        res = upload_response_actions_file(region=region, env=env, tenant_id=tenant_id, endpoint_id=get_endpoint_id(),
                                           file_path=filepath)
        logging.debug(res)

        action_status = get_response_action_status(region=region, env=env, tenant_id=tenant_id, action_id=res["id"])
        logging.debug(action_status)

        if action_status["endpoints"][0]["result"] != "succeeded":
            logging.warning(f"Failed to upload response actions file {filepath}")
            failed_file_uploads += 1
    end_time = get_current_unix_epoch_in_seconds()

    record_result("Response Actions Upload Files", get_current_date_time_string(), start_time, end_time,
                  custom_data={"failed_file_uploads": failed_file_uploads})

    if failed_file_uploads == 10:
        logging.error(f"Failed to upload all response actions files")
        return_code = Jenkins_Job_Return_Code.FAILURE
    elif 0 < failed_file_uploads:
        logging.warning(f"Failed to upload response actions files {failed_file_uploads} times")
        return_code = Jenkins_Job_Return_Code.UNSTABLE
    exit(return_code)


def run_response_actions_list_files_test(region, env, tenant_id):
    return_code = Jenkins_Job_Return_Code.SUCCESS
    matching_dir_content = 0
    expected_dir_content = set(f for f in os.listdir("/home/pair") if not f.startswith("."))

    start_time = get_current_unix_epoch_in_seconds()
    for i in range(10):
        res = send_execute_command(region=region, env=env, tenant_id=tenant_id, endpoint_id=get_endpoint_id(),
                                   cmd="ls /home/pair")
        logging.debug(res)

        action_id = res["id"]
        action_status = get_response_action_status(region=region, env=env, tenant_id=tenant_id, action_id=action_id)
        logging.debug(action_status)

        if action_status["endpoints"][0]["output"]["stdOut"]:
            cmd_output = action_status["endpoints"][0]["output"]["stdOut"]
            logging.debug(f"Command output: {cmd_output}")
            if set(cmd_output.strip().split("\n")) == expected_dir_content:
                matching_dir_content += 1
    end_time = get_current_unix_epoch_in_seconds()

    record_result("Response Actions List Files", get_current_date_time_string(), start_time, end_time)

    if matching_dir_content == 0:
        logging.error(f"Command output did not match expected directory content: {expected_dir_content}")
        return_code = Jenkins_Job_Return_Code.FAILURE
    elif matching_dir_content < 10:
        logging.warning(
            f"Command output did not match expected directory content {matching_dir_content} times: {expected_dir_content}")
        return_code = Jenkins_Job_Return_Code.UNSTABLE
    exit(return_code)


def add_options():
    parser = argparse.ArgumentParser(description='Performance test runner for EDR')

    parser.add_argument('-s', '--suite', action='store',
                        choices=['gcc',
                                 'local-livequery',
                                 'local-livequery-detections',
                                 'central-livequery',
                                 'local-liveresponse_x1',
                                 'local-liveresponse_x10',
                                 'event-journaler-ingestion',
                                 'av-onaccess-slow',
                                 'av-onaccess',
                                 'onaccess-performance',
                                 'safestore-confirm-detections',
                                 'safestore-restore-database',
                                 'ra-file-download',
                                 'ra-file-upload',
                                 'ra-list-files',
                                 'add-annotations',
                                 'delete-annotations'],
                        help="Select which performance test suite to run")

    parser.add_argument('-i', '--client-id', action='store',
                        help="Central account API client ID to use to run live queries")

    parser.add_argument('-t', '--tenant-id', action='store',
                        help="Central account API tenant ID to use for response actions")

    parser.add_argument('-e', '--email', default='darwinperformance@sophos.xmas.testqa.com', action='store',
                        help="Central account email address to use to run live queries")

    parser.add_argument('-p', '--password', action='store',
                        help="Central account API client secret or password to use to run live queries")

    parser.add_argument('-c', '--central-env', action='store', help="Central environment")

    parser.add_argument('-r', '--central-region', action='store', help="Central region")

    parser.add_argument('-d', '--dry_run', action='store_true', default=False,
                        help="Run tests locally without storing results")

    parser.add_argument('-m', '--max-file-count', type=int, default=100000, action='store',
                        help="Maximum number of files to create for the av-onaccess test")
    parser.add_argument('-M', '--max-file-count-slow', type=int, default=1000, action='store',
                        help="Maximum number of files to create for the av-onaccess-slow test")

    parser.add_argument('--on-access-read', action='store_true',
                        help="Toggle On-Access On-Read on for onaccess-performance test")
    parser.add_argument('--no-on-access-read', dest='on-access-read', action='store_false',
                        help="Toggle On-Access On-Read off for onaccess-performance test")

    parser.add_argument('--on-access-write', action='store_true',
                        help="Toggle On-Access On-Write on for onaccess-performance test")
    parser.add_argument('--no-on-access-write', dest='on-access-write', action='store_false',
                        help="Toggle On-Access On-Write off for onaccess-performance test")

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
        run_central_live_query_perf_test(args.client_id, args.email, args.password, args.central_env)
    elif args.suite == 'local-liveresponse_x1':
        run_local_live_response_test(1, 0)
        run_local_live_response_test(1, 300)
    elif args.suite == 'local-liveresponse_x10':
        run_local_live_response_test(10, 0)
    elif args.suite == 'event-journaler-ingestion':
        run_event_journaler_ingestion_test()
    elif args.suite == 'av-onaccess':
        run_onaccess_test(args.max_file_count)
    elif args.suite == 'av-onaccess-slow':
        run_slow_scan_onaccess_test(args.max_file_count_slow)
    elif args.suite == 'onaccess-performance':
        run_on_access_performance_test(args.client_id, args.password, args.central_env, args.on_access_write,
                                       args.on_access_read)
    elif args.suite == 'safestore-confirm-detections':
        run_safestore_database_content_test()
    elif args.suite == 'safestore-restore-database':
        run_safestore_restoration_test()
    elif args.suite == 'ra-file-download':
        run_response_actions_download_files_test(args.central_region, args.central_env, args.tenant_id)
    elif args.suite == 'ra-file-upload':
        run_response_actions_upload_files_test(args.central_region, args.central_env, args.tenant_id)
    elif args.suite == 'ra-list-files':
        run_response_actions_list_files_test(args.central_region, args.central_env, args.tenant_id)
    elif args.suite == 'add-annotations':
        annotate_graphs()
    elif args.suite == 'delete-annotations':
        delete_annotations(dry_run)
    logging.info("Finished")


if __name__ == '__main__':
    sys.exit(main())
