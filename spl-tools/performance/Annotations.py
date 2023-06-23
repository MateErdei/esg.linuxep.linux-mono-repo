import socket

import requests

from LogUtils import *


def get_grafana_auth():
    with open("/root/performance/grafana_token") as f:
        return {
            "Authorization": f"Bearer {f.read}"
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
    else:
        print(f"Annotation Content: {annotation_json}")


def add_product_update_annotations():
    update_info = get_chunks_from_log(LogUtils.LogUtils.suldownloader_log,
                                      "Doing product and supplement update",
                                      "Generating the report file in")
    for update_chunk in update_info:
        # Ignore hourly updates where product is not upgraded
        if "Product installed" not in "".join(update_chunk):
            continue
        update_info_text = "Product Update:\n"
        start_time = get_epoch_time_from_log_line(update_chunk[0])
        end_time = get_epoch_time_from_log_line(update_chunk[-1])
        for log_line in update_chunk:
            for component in next(os.walk("/opt/sophos-spl/base/update/cache/sdds3primary"))[1]:
                if f"Installing product: {component} version:" in log_line:
                    component_version = log_line.split(":")[-1].strip()
                    update_info_text += f"Upgraded {component} to {component_version}\n"

        add_annotation(tag="product-upgrade", start_time=start_time, end_time=end_time, text=update_info_text)


def add_scheduled_scan_annotations():
    scan_info = get_chunks_from_log(LogUtils.LogUtils.av_log,
                                    "Starting scan Sophos Cloud Scheduled Scan",
                                    "Sending scan complete event to Central")

    for scan_chunk in scan_info:
        start_time = get_epoch_time_from_log_line(scan_chunk[0])
        end_time = get_epoch_time_from_log_line(scan_chunk[-1])
        add_annotation(tag="scheduled-scan", start_time=start_time, end_time=end_time, text="Scheduled Scan")


def annotate_graphs():
    add_product_update_annotations()
    add_scheduled_scan_annotations()
