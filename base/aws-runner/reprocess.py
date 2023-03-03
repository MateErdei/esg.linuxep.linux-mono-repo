#!/usr/bin/env python3
# Copyright 2023 Sophos Limited. All rights reserved.

import datetime
import json
import os
import shutil
import subprocess
import sys
import time
import xml.etree.ElementTree


def replace_top_level_suite(output_xml_file):
    # python3 -m robot.rebot --merge -o ./results-combine-workspace/$B -l none -r none -N combined  $F
    basename = os.path.basename(output_xml_file)
    dest = os.path.join("results-combine-workspace", basename)
    if os.path.isfile(dest):
        return dest

    assert basename.endswith("-output.xml")
    hostname = basename[:-len("-output.xml")]
    command = ["python3", "-m", "robot.rebot", "--merge", "-o", dest, "-l", "none", "-r", "none",
               "-N", "combined", output_xml_file]
    print(" ".join(command))
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if not os.path.isfile(dest):
        print("Copying", basename, "to ensure it is present in results-combine-workspace")
        shutil.copyfile(output_xml_file, dest)  # ensure we have something to work with
        if result.returncode == 0:
            result.returncode = -1
    if result.returncode != 0:
        print("robot.rebot failed for", basename, "with exit code", result.returncode)
        print("Output:", str(result.stdout))

    assert os.path.isfile(dest)
    return dest


def duration(starttime, endtime):
    fmt = "%Y%m%d %H:%M:%S.%f"
    start = datetime.datetime.strptime(starttime, fmt)
    end =   datetime.datetime.strptime(endtime, fmt)
    d = end - start
    return d


def save_summary(output_xml_file):
    result_dir, basename = os.path.split(output_xml_file)
    assert basename.endswith("-output.xml")
    hostname = basename[:-len("-output.xml")]
    dest = os.path.join(result_dir, hostname+"-summary.json")
    if os.path.isfile(dest):
        return dest

    result = {
        "hostname": hostname
    }

    tree = xml.etree.ElementTree.parse(output_xml_file)
    statuses = tree.findall("./suite/status")
    if len(statuses) == 1:
        status = statuses[0]
        fullstarttime = status.get("starttime")
        fullendtime = status.get("endtime")
        d = duration(fullstarttime, fullendtime)
        result['duration'] = str(d)
        result['durationSeconds'] = d.total_seconds()

        result["full-start-time"] = fullstarttime
        result["full-end-time"] = fullendtime
        starttime = fullstarttime[12:]
        result["start-time"] = starttime
        endtime = fullendtime[12:]
        result["end-time"] = endtime

    print(result)
    json.dump(result, open(dest, "w"), indent=4)
    return dest


def reprocess(output_xml_file):
    replace_top_level_suite(output_xml_file)
    return save_summary(output_xml_file)


def print_summary(summaries):
    summaries = [ json.load(open(s)) for s in summaries ]

    print("By Duration")
    summaries.sort(key=lambda x: x['durationSeconds'])
    for s in summaries:
        print(s['duration'], s['hostname'], s.get('arguments', ""))

    print("\nBy Start Time")
    summaries.sort(key=lambda x: x['start-time'])
    for s in summaries:
        print(s['full-start-time'], s['hostname'], s.get('arguments', ""))

    print("\nBy End Time")
    summaries.sort(key=lambda x: x['end-time'])
    for s in summaries:
        print(s['full-end-time'], s['hostname'])


def main(argv):
    summaries = []
    for output_xml_file in argv[1:]:
        try:
            summaries.append(reprocess(output_xml_file))
        except Exception as ex:
            print("Got exception but carrying on", str(ex))
    print_summary(summaries)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
