import datetime
import logging
import os
import socket
import requests

import LogUtils

requests.packages.urllib3.disable_warnings(requests.packages.urllib3.exceptions.InsecureRequestWarning)

LOG_UTILS = LogUtils.LogUtils()

COMPONENT_NAMES_TO_FORMAT = {
    "ServerProtectionLinux-Base-component": "Base",
    "ServerProtectionLinux-Plugin-responseactions": "Response Actions",
    "ServerProtectionLinux-Plugin-liveresponse": "Live Response",
    "ServerProtectionLinux-Plugin-RuntimeDetections": "RTD",
    "ServerProtectionLinux-Plugin-MDR": "MDR",
    "ServerProtectionLinux-Plugin-EventJournaler": "Event Journaler",
    "ServerProtectionLinux-Plugin-EDR": "EDR",
    "ServerProtectionLinux-Plugin-AV": "AV"
}

API_HOST = "https://sspl-alex-1.eng.sophos:3000"


def get_grafana_auth():
    with open("/root/performance/grafana_token") as f:
        return {
            "Authorization": f"Bearer {f.read().strip()}"
        }


def add_annotation(tag, start_time, text, end_time=None):
    hostname = socket.gethostname()

    headers = get_grafana_auth()
    r = requests.get(f"{API_HOST}/api/annotations?tags={hostname}&tags={tag}", headers=headers,
                     verify=False)
    for i in r.json():
        # prevent duplicate annotations being created
        if i["time"] == start_time and i["text"] == text:
            logging.debug(f"Not sending duplicate annotation (id={i['id']}) at {start_time}: {text}")
            return 0

    annotation_json = {
        "time": start_time,
        "timeEnd": end_time if end_time else start_time,

        "tags": [hostname, tag],
        "text": text
    }

    r = requests.post(f'{API_HOST}/api/annotations',
                      headers=headers,
                      json=annotation_json,
                      verify=False)
    if r.status_code not in [200, 201]:
        logging.error(f"Failed to store test result: {str(annotation_json)}")
        logging.error(f"Status code: {r.status_code}, text: {r.text}")
        return 1
    else:
        logging.info(f"Annotation Content: {annotation_json}")
        return 0


def add_product_update_annotations():
    annotation_failures = 0
    update_info = LogUtils.get_chunks_from_log(LOG_UTILS.suldownloader_log,
                                               "Doing product and supplement update",
                                               "Generating the report file in")
    for update_chunk in update_info:
        # Ignore hourly updates where product is not upgraded
        if "Product installed" not in "".join(update_chunk):
            continue
        update_info_text = "Product Update:\n"
        start_time = LogUtils.get_epoch_time_from_log_line(update_chunk[0])
        end_time = LogUtils.get_epoch_time_from_log_line(update_chunk[-1])
        for log_line in update_chunk:
            for component in next(os.walk(f"{LOG_UTILS.install_path}/base/update/cache/sdds3primary"))[1]:
                if f"Installing product: {component} version:" in log_line:
                    component_name = COMPONENT_NAMES_TO_FORMAT[component] \
                        if component in COMPONENT_NAMES_TO_FORMAT else component
                    component_version = log_line.split(":")[-1].strip()
                    update_info_text += f"Upgraded {component_name} to {component_version}\n"

        annotation_failures += add_annotation(tag="product-upgrade",
                                              start_time=start_time,
                                              end_time=end_time,
                                              text=update_info_text)
    return annotation_failures


def add_scheduled_scan_annotations():
    annotation_failures = 0
    scan_info = LogUtils.get_chunks_from_log(LOG_UTILS.av_log,
                                             "Starting scan Sophos Cloud Scheduled Scan",
                                             "Sending scan complete event to Central")

    for scan_chunk in scan_info:
        start_time = LogUtils.get_epoch_time_from_log_line(scan_chunk[0])
        end_time = LogUtils.get_epoch_time_from_log_line(scan_chunk[-1])
        duration_in_min = round(((end_time - start_time) / (1000 * 60)) % 60, 2)
        annotation_failures += add_annotation(tag="scheduled-scan",
                                              start_time=start_time,
                                              end_time=end_time,
                                              text=f"Scheduled Scan (took {duration_in_min} minutes)")
    return annotation_failures


def add_osquery_restart_annotations():
    # Note: every PID restart shown on Grafana can technically be related to an "executing worker (<new pid>)" log line
    # But the checks below try to add an annotation that can specify why the restart happened as well

    annotation_failures = 0
    logs_dir_path = os.path.dirname(LOG_UTILS.edr_log)
    # Directory contains a downgrade-backup subdirectory that we want to ignore, so only want os.walk() to yield once
    log_files_names = next(os.walk(logs_dir_path), (None, None, []))[2]

    for log_file_name in log_files_names:
        with open(os.path.join(logs_dir_path, log_file_name)) as f:
            log_file_data = f.readlines()

            for line in log_file_data:
                # ---- In edr logs ----
                # Other restarts
                if "edr <> Restarting osquery" in line:  # Technically EDR is trying to start OSQuery not "restart"
                    annotation_failures += add_annotation(tag="osquery-restart",
                                                          start_time=LogUtils.get_epoch_time_from_log_line(line),
                                                          text="EDR trying to start osquery")
                elif "Failed to reconfigure osquery" in line:
                    annotation_failures += add_annotation(tag="osquery-restart",
                                                          start_time=LogUtils.get_epoch_time_from_log_line(line),
                                                          text="Failed to reconfigure osquery")
                elif "Stopping EDR" in line:
                    annotation_failures += add_annotation(tag="osquery-restart",
                                                          start_time=LogUtils.get_epoch_time_from_log_line(line),
                                                          text="Stopping EDR")
                elif "Restarting osquery, reason: " in line:
                    annotation_failures += add_annotation(tag="osquery-restart",
                                                          start_time=LogUtils.get_epoch_time_from_log_line(line),
                                                          text=line.split("reason: ")[-1])
                # Memory restarts
                elif "Memory usage exceeded" in line:
                    # In edr logs
                    annotation_failures += add_annotation(tag="osquery-memory-restart",
                                                          start_time=LogUtils.get_epoch_time_from_log_line(line),
                                                          text=line.split("stopping, ")[-1])

                # ---- In edr_osquery logs ----
                # Other restarts
                elif "An error occurred during extension manager startup" in line:
                    annotation_failures += add_annotation(tag="osquery-restart",
                                                          start_time=LogUtils.get_epoch_time_from_log_line(line),
                                                          text=line.split("startup: ")[-1])
                elif "Refreshing configuration state" in line:
                    # At the time the config given to OSQuery specified a "--config_refresh" flag set to 3600
                    # So if OSQuery doesn't have to restart for an hour, the config refresh will cause a PID change
                    annotation_failures += add_annotation(tag="osquery-restart",
                                                          start_time=LogUtils.get_epoch_time_from_log_line(line),
                                                          text="Refreshing configuration state")

                # CPU restarts
                elif "Maximum sustainable CPU utilization limit exceeded" in line:
                    annotation_failures += add_annotation(tag="osquery-cpu-restart",
                                                          start_time=LogUtils.get_epoch_time_from_log_line(line),
                                                          text=line.split("stopping: ")[-1])

                # Memory restarts
                elif "Memory limits exceeded" in line:
                    annotation_failures += add_annotation(tag="osquery-memory-restart",
                                                          start_time=LogUtils.get_epoch_time_from_log_line(line),
                                                          text=line.split("stopping: ")[-1])

    return annotation_failures


def check_duplicate_annotation_does_not_exist_and_get_id(tag):
    hostname = socket.gethostname()
    headers = get_grafana_auth()
    r = requests.get(f"{API_HOST}/api/annotations?tags={hostname}&tags={tag}", headers=headers,
                     verify=False)

    # Need to make sure that if on-read or on-write was "turned on" twice, another annotation is not added
    # E.g. "On-Write On, On-Read Off" then "On-Write on, On-Read On" tests are ran one after the other, don't want to
    # add two annotations for On-Write On but just one

    annotation_id = None
    for i in r.json():
        if i["time"] == i["timeEnd"]:
            if annotation_id is not None:
                logging.error("There are multiple annotations with the same start time and end time."
                              "This should not be the case, there should only be one annotation (relating to"
                              "on-access-read/on-access-write tag respectively) with the same start and end time")
                return 1
            else:
                annotation_id = i["id"]
                # Don't break out of loop to make sure that this annotation is the only one with the same start/end time

    return annotation_id


def add_onaccess_performance_test_annotation(tag, start_time, text, end_time=None):
    annotation_id = check_duplicate_annotation_does_not_exist_and_get_id(tag)
    if annotation_id is None:
        # Aka on-access setting was turned off previously and now turned on
        logging.debug(f"Adding new annotation for on-access setting with tag: {tag}")
        return add_annotation(tag=tag, start_time=start_time, text=text, end_time=end_time)
    else:
        # Annotation was turned on "twice"
        logging.debug("'Second' time on-access setting would be turned on, not adding a new annotation")
        return 0


def patch_onaccess_performance_test_annotation(tag, end_time):
    annotation_id = check_duplicate_annotation_does_not_exist_and_get_id(tag)
    if annotation_id is None:
        logging.warning(f"Found no annotations with tag: {tag} with same start and end times")
        return 2

    patch_json = {
        "timeEnd": end_time
    }

    headers = get_grafana_auth()
    r = requests.patch(f"{API_HOST}/api/annotations/{annotation_id}", json=patch_json,
                       headers=headers, verify=False)

    if r.status_code != 200:
        logging.error(f"Failed to patch timeEnd value for annotation with id: {annotation_id}, tag: {tag}."
                      f"timeEnd: {end_time}"
                      f"response json: {r.json()}")
        return 1

    return 0


def annotate_graphs():
    annotation_failures = 0
    annotation_failures += add_product_update_annotations() + add_scheduled_scan_annotations() + add_osquery_restart_annotations()

    if annotation_failures != 0:
        exit(1)


def delete_annotations(dry_run):
    headers = get_grafana_auth()

    time_now = datetime.datetime.now()
    two_weeks_ago = time_now - datetime.timedelta(weeks=2)
    one_year_ago = time_now - datetime.timedelta(weeks=52)
    time_to_delete_two_weeks_ago = int(two_weeks_ago.timestamp() * 1000)
    time_to_delete_one_year_ago = int(one_year_ago.timestamp() * 1000)

    r = requests.get(
        f"{API_HOST}/api/annotations?from={time_to_delete_one_year_ago}&to={time_to_delete_two_weeks_ago}&tags={socket.gethostname()}&limit=10000",
        headers=headers,
        verify=False
    )

    if dry_run:
        logging.info("DRY RUN: Would delete the following annotations")
        logging.info(r.json())
        return

    for i in r.json():
        res = requests.delete(f'{API_HOST}/api/annotations/{i["id"]}', headers=headers,
                              verify=False)
        if res.status_code not in [200, 201]:
            logging.error(f"Failed to delete annotation: {r.text}")
