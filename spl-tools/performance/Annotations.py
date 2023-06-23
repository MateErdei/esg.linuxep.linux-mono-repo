import os
import socket

import requests

import LogUtils

LOG_UTILS = LogUtils.LogUtils()


def get_grafana_auth():
    with open("/root/performance/grafana_token") as f:
        return {
            "Authorization": f"Bearer {f.read().strip()}"
        }


def add_annotation(tag, start_time, text, end_time=None):
    hostname = socket.gethostname()

    annotation_json = {
        "time": start_time,
        "timeEnd": end_time if end_time else start_time,
        "tags": [hostname, tag, "test"],
        "text": text
    }

    r = requests.post('https://sspl-alex-1.eng.sophos:3000/api/annotations', headers=get_grafana_auth(),
                      json=annotation_json, verify=False)
    if r.status_code not in [200, 201]:
        print(f"Failed to store test result: {str(annotation_json)}")
        print(f"Status code: {r.status_code}, text: {r.text}")
        return 1
    else:
        print(f"Annotation Content: {annotation_json}")
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
                    component_version = log_line.split(":")[-1].strip()
                    update_info_text += f"Upgraded {component} to {component_version}\n"

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
        annotation_failures += add_annotation(tag="scheduled-scan",
                                              start_time=start_time,
                                              end_time=end_time,
                                              text="Scheduled Scan")
    return annotation_failures


def annotate_graphs():
    annotation_failures = 0
    annotation_failures += add_product_update_annotations() + \
                           add_scheduled_scan_annotations()

    if annotation_failures != 0:
        exit(1)
