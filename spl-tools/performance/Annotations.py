import datetime
import logging
import os
import socket
import subprocess

import requests

import LogUtils

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


def get_grafana_auth():
    with open("/root/performance/grafana_token") as f:
        return {
            "Authorization": f"Bearer {f.read().strip()}"
        }


def get_epoch_time_from_journal_entry(journal_line):
    time_string = f"{datetime.date.today().year} {journal_line.split(socket.gethostname())[0].strip()}"
    return int(datetime.datetime.strptime(time_string, "%Y %b %d %H:%M:%S").timestamp() * 1000)  # in milliseconds


def add_annotation(tag, start_time, text, end_time=None):
    hostname = socket.gethostname()

    headers = get_grafana_auth()
    r = requests.get(f"https://sspl-alex-1.eng.sophos:3000/api/annotations?tags={hostname}&tags={tag}", headers=headers,
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

    r = requests.post('https://sspl-alex-1.eng.sophos:3000/api/annotations',
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
    annotation_failures = 0
    with open(LOG_UTILS.edr_log) as f:
        lines = f.readlines()

        for line in lines:
            if "Restarting osquery, reason:" in line:
                annotation_failures += add_annotation(tag="osquery-restart",
                                                      start_time=LogUtils.get_epoch_time_from_log_line(line),
                                                      text=line.split('reason:')[-1])
            if "Restarting OSQuery after unexpected extension exit" in line:
                annotation_failures += add_annotation(tag="osquery-restart",
                                                      start_time=LogUtils.get_epoch_time_from_log_line(line),
                                                      text="OSQuery Restart due to unexpected extension exit")

    ps = subprocess.run(["journalctl", "-u", "sophos-spl"], check=True, capture_output=True)

    cpu_limit_data = subprocess.run(["grep", "Maximum sustainable CPU utilization limit exceeded"],
                                    input=ps.stdout, capture_output=True).stdout.decode("utf-8").strip().split("\n")
    for data in cpu_limit_data:
        if data:
            annotation_failures += add_annotation(tag="osquery-cpu-restart",
                                                  start_time=get_epoch_time_from_journal_entry(data),
                                                  text=data.split("stopping: ")[-1])

    memory_limit_data = subprocess.run(["grep", "Memory limits exceeded"],
                                       input=ps.stdout, capture_output=True).stdout.decode("utf-8").strip().split("\n")
    for data in memory_limit_data:
        if data:
            annotation_failures += add_annotation(tag="osquery-memory-restart",
                                                  start_time=get_epoch_time_from_journal_entry(data),
                                                  text=data.split("stopping: ")[-1])

    return annotation_failures


def annotate_graphs():
    requests.packages.urllib3.disable_warnings(requests.packages.urllib3.exceptions.InsecureRequestWarning)
    annotation_failures = 0
    annotation_failures += add_product_update_annotations() + add_scheduled_scan_annotations() + add_osquery_restart_annotations()

    if annotation_failures != 0:
        exit(1)


def delete_annotations(dry_run):
    requests.packages.urllib3.disable_warnings(requests.packages.urllib3.exceptions.InsecureRequestWarning)
    headers = get_grafana_auth()

    two_weeks_ago = datetime.datetime.now() - datetime.timedelta(weeks=2)
    time_to_delete = int(two_weeks_ago.timestamp() * 1000)

    r = requests.get(
        f"https://sspl-alex-1.eng.sophos:3000/api/annotations?to={time_to_delete}&tags={socket.gethostname()}&limit=10000",
        headers=headers,
        verify=False
    )

    if dry_run:
        logging.info("DRY RUN: Would delete the following annotations")
        logging.info(r.json())
        return

    for i in r.json():
        res = requests.delete(f'https://sspl-alex-1.eng.sophos:3000/api/annotations/{i["id"]}', headers=headers,
                              verify=False)
        if res.status_code not in [200, 201]:
            logging.error(f"Failed to delete annotation: {r.text}")
