# Copyright 2019, Sophos Limited.  All rights reserved.

# This script is intended to be used to import diagnose tars into the dogfood logs DB.

import sys
if sys.version_info[0] < 3:
    raise Exception("Must be using Python 3")

import time
import os
import re
from pathlib import Path
import datetime
import shutil
import io

try:
    import mysql.connector
except:
    print("No module named 'mysql', please install it: python3 -m pip install mysql-connector-python")
    exit()

try:
    import magic
except:
    print("No module named 'magic', please install it: python3 -m pip install python-magic")
    exit()

try:
    import xmltodict
except:
    print("No module named 'xmltodict', please install it: python3 -m pip install xmltodict")
    exit()


def get_time_string_from_log_line(line):
    global g_product

    t = ""
    if g_product == "SAV":
        # 2019-06-18 14:12:29
        pattern = "[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9]"
        reg = re.compile(pattern)
        try:
            t = reg.findall(line)[0]
        except:
            print("Could not find date in:{}".format(line))
            return ""
    elif g_product == "SSPL":
        # [2019-06-18T13:14:02.528]
        pattern = "\[[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9]\.[0-9][0-9][0-9]\]"
        reg = re.compile(pattern)
        try:
            t = reg.findall(line)[0]
        except:
            print("Could not find date in:{}".format(line))
            return ""
        t = t.replace('T', ' ')
        t = t.replace('[', '')
        t = t.replace(']', '')
        t = t.split('.')[0]
    return t


def send_log_line_to_db(line, log_path, db, ip, hostname, latest_time, last_id, product_base_version):
    line = line.strip()

    # Empty lines means a blank line is in the log file, so add one here.
    if line == "":
        line = "\n"

    cursor = db.cursor()
    extracted_time = get_time_string_from_log_line(line)

    # if extracted_time is "" then the log line probably wasn't logged as a single line and thus did not have a timestamp in it.
    # So append this line to last committed line.
    if extracted_time == "":
        if last_id is None:
            # if here then we've started capturing logs half way through a multi line log write
            # just throw the line away until we get the start of complete log line with a timestamp in it.
            return last_id

        df_sql = "UPDATE dogfood_logs SET log_msg = CONCAT(log_msg, '\n', %s) WHERE idlogs = %s"
        cursor.execute(df_sql, (line, last_id))
        db.commit()

    else:

        if latest_time > datetime.datetime.strptime(extracted_time, '%Y-%m-%d %H:%M:%S'):
            print("Not inserting log line we have already seen from: {}".format(extracted_time))
            return last_id

        df_sql = "INSERT INTO dogfood_logs (log_time, log_msg, log_path, log_name, hostname, ip, base_version) VALUES (%s, %s, %s, %s, %s, %s, %s)"
        df_val = (extracted_time, line, "diagnose", os.path.basename(log_path), hostname, ip, product_base_version)
        cursor.execute(df_sql, df_val)
        db.commit()
        last_id = cursor.lastrowid

    print("inserted, id:{}".format(last_id))
    return last_id


def send_system_file_line_to_db(line, log_path, db, ip, hostname):
    line = line.strip()

    cursor = db.cursor()

    #TODO for files where it is possible extract a log line time.
    # for now set this to something default, we won't set it to anything as most system files don't have the same
    # structure as a log file, e.g. a log line with a timestamp.
    log_time = "2019-01-01T00:00:00.000"

    try:
        df_sql = "CALL insert_sys_file_line(%s, %s, %s, %s, %s, %s)"
        df_val = (log_time, line, log_path, os.path.basename(log_path), hostname, ip)
        cursor.execute(df_sql, df_val)
        db.commit()
        last_id = cursor.lastrowid
        print("inserted, id:{}".format(last_id))
    except Exception as e:
        print("Exception for: {} ".format(line))
        print(e)


def usage():
    print("Wrong arguments. Usage below:")
    this_script_name = os.path.basename(__file__)

    print("python3 {} [product] [path-to-search-for-.tar.gz]".format(this_script_name))
    print("Supported products: SSPL")
    print("Examples:")
    print("python3 {} SSPL /mnt/filer6/linux/SSPLDogfood/".format(this_script_name))


def is_sspl_diagnose_file(path):
    return os.path.basename(path).startswith("sspl-diagnose") and path.endswith(".tar.gz")


def tar_already_processed(tar_path):
    tar_dir = os.path.dirname(tar_path)
    tar_name = os.path.basename(tar_path)
    marker_file_name = tar_name + ".done"
    return os.path.exists(os.path.join(tar_dir, marker_file_name))


def mark_tar_as_processed(tar_path):
    marker_file_path = tar_path + ".done"
    with open(marker_file_path, 'w') as marker_file:
        ts = time.time()
        pretty_date = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
        marker_file.write(pretty_date)
    print("Finished with:{}".format(tar_path))


def extract_part(extracted_tar_path, filename, tag, delim, position):
    with open(os.path.join(extracted_tar_path, "SystemFiles", filename), 'r') as file:
        for line in file:
            if "timed out" in line.lower():
                return None
            if tag in line or tag == "":
                x = line.split(delim)
                if len(x) > 1:
                    hostname = x[position].strip()
                    if hostname is not "":
                        return hostname
    return None


def extract_hostname(extracted_tar_path):
    hostname = extract_part(extracted_tar_path, "hostnamectl", "hostname", ":", 1)
    if hostname is not None:
        return hostname

    hostname = extract_part(extracted_tar_path, "sysctl", "kernel.hostname", " = ", 1)
    if hostname is not None:
        return hostname

    hostname = extract_part(extracted_tar_path, "uname", "", " ", 1)
    if hostname is not None:
        return hostname


# todo make this more robust around splitting.
def extract_ip(extracted_tar_path):
    ips_to_ignore = ["127.0.0.1", "::1"]
    with open(os.path.join(extracted_tar_path, "SystemFiles", "ip-addr"), 'r') as ip_addr:
        for line in ip_addr:
            if "inet" in line:
                line = line.strip()
                x = line.split(" ")
                ip = x[1].split("/")[0]

                if ip in ips_to_ignore:
                    continue
                return ip


# VERSION.INI is not in dogfood yet so we have to get product version from somewhere else.
def extract_base_version(extracted_tar_path):
    with open(os.path.join(extracted_tar_path, "BaseFiles", "ALC_status.xml"), 'r') as alc_status:
        status = xmltodict.parse(alc_status.read(), dict_constructor=dict)
        subscriptions = status['status']['subscriptions']['subscription']

        for subscription in subscriptions:
            if subscription['@rigidName'] == "ServerProtectionLinux-Base":
                return subscription['@version']
    return "unknown"


def process_diagnose_file(tar_path):
    global g_extract_dir

    print(tar_path)
    cleanup()

    if not os.path.isdir(g_extract_dir):
        os.mkdir(g_extract_dir)

    tar_file_name = os.path.basename(tar_path)
    local_tar_path = os.path.join(g_extract_dir, tar_file_name)

    if os.path.exists(local_tar_path):
        print("There is a diagnose tar with a clashing name: {}".format(local_tar_path))
        exit(1)

    shutil.copyfile(tar_path, local_tar_path)

    sub_dir = os.path.join(g_extract_dir, tar_file_name + ".dir")

    import tarfile
    tar = tarfile.open(local_tar_path)
    tar.extractall(sub_dir)
    tar.close()

    # product logs
    logs = []

    # misc logs, e.g. system files - these do not have a set format so will just treat them as text and use insert time.
    system_files = []

    # Base
    base = os.path.join(sub_dir, "BaseFiles")
    for filename in Path(base).glob('**/*.log*'):
        logs.append(str(filename))
        print(filename)

    # Plugins
    plugins = os.path.join(sub_dir, "PluginFiles")
    for filename in Path(plugins).glob('**/*.log*'):
        logs.append(str(filename))
        print(filename)

    # System files - NB currently ignoring audit log as it is so verbose.
    system_file_path = os.path.join(sub_dir, "SystemFiles")
    for filename in Path(system_file_path).glob('*'):
        if "audit.log" not in str(filename):
            system_files.append(str(filename))
        print(filename)

    hostname = extract_hostname(sub_dir)
    if hostname is None or hostname == "None" or hostname == "":
        print("Skipping tar because could not extract hostname from hostnamectl: {}".format(tar_path))
        return

    ip = extract_ip(sub_dir)
    print("Hostname: {}".format(hostname))
    print("IP: {}".format(ip))

    product_base_version = extract_base_version(sub_dir)

    # This account has only insert privileges (not even select) so it is safe to include here.
    # Dashboard sanitizes html so a script being inserted for example will not do anything when displayed.
    dogfood_db = mysql.connector.connect(
        host="10.55.36.229",
        user="dogfoodrw",
        passwd="mason-R8Chu",
        database="dogfood"
    )

    # TODO LINUXEP-8365 When importing don't do it one log line at a time do it in bulk.
    for log in logs:
        process_log_file(hostname, dogfood_db, ip, log, product_base_version)

    # for sys_file in system_files:
    #     process_system_file(hostname, dogfood_db, ip, sys_file)

    mark_tar_as_processed(tar_path)


def get_latest_log_time_from_db(hostname, db, log_name):
    cursor = db.cursor()
    df_sql = "SELECT MAX(log_time) FROM dogfood_logs WHERE hostname = %s AND log_name = %s"
    df_val = (hostname, log_name)
    cursor.execute(df_sql, df_val)
    results = cursor.fetchall()

    # Default if more than one, there shold never be.
    if len(results) != 1:
        min_time = datetime.date.min
        return datetime.datetime(min_time.year, min_time.month, min_time.day)

    if results[0] is not None and results[0][0] is not None:
        return results[0][0]

    min_time = datetime.date.min
    return datetime.datetime(min_time.year, min_time.month, min_time.day)


def process_log_file(hostname, db, ip, log_file_path, product_base_version):
    print("Processing log:{}".format(log_file_path))
    if 'text' not in magic.from_file(log_file_path):
        print("Log file is not a text file, skipping it.")
        return

    latest_time = get_latest_log_time_from_db(hostname, db, os.path.basename(log_file_path))
    print(latest_time)

    last_id = None

    with io.open(log_file_path, mode="r", encoding="utf-8") as log:
        for line in log:
            last_id = send_log_line_to_db(line, log_file_path, db, ip, hostname, latest_time, last_id, product_base_version)


def process_system_file(hostname, db, ip, sys_file_path):
    print("Processing system file:{}".format(sys_file_path))
    if 'text' not in magic.from_file(sys_file_path):
        print("Log file is not a text file, skipping it.")
        return

    with io.open(sys_file_path, mode="r", encoding="utf-8") as log:
        for line in log:
            send_system_file_line_to_db(line, sys_file_path, db, ip, hostname)


def cleanup():
    if g_extract_dir.startswith("/tmp/") and os.path.exists(g_extract_dir):
        shutil.rmtree(g_extract_dir)


# Globals
g_product = "SSPL"
g_extract_dir = "/tmp/dogfood-extract"


def main():

    global g_product
    global g_extract_dir

    if len(sys.argv) != 3:
        usage()
        exit(1)

    print("Product: {}".format(g_product))

    tars = []
    tar_path = sys.argv[2]
    if not os.path.exists(tar_path):
        print("Path does not exist: {}".format(tar_path))
        exit(1)

    if os.path.isdir(tar_path):
        for filename in Path(tar_path).glob('**/sspl-diagnose*.tar.gz'):
            tars.append(str(filename))
    elif os.path.isfile(tar_path):
        if is_sspl_diagnose_file(tar_path):
            tars.append(tar_path)
        else:
            print("The file: {}, does not start with sspl-diagnose and end with .tar.gz, so assuming it is not an SSPL dog food tar.gz file.",format(tar_path))
    else:
        exit(1)

    tars_to_process = [x for x in tars if not tar_already_processed(x)]

    skip_list = set(tars) - set(tars_to_process)
    for skipped_tar in skip_list:
        print("Skipping because already processed: {}".format(skipped_tar))

    for tar in tars_to_process:
        process_diagnose_file(tar)

    # if g_extract_dir.startswith("/tmp/") and os.path.exists(g_extract_dir):
    #     shutil.rmtree(g_extract_dir)
    cleanup()


if __name__ == "__main__":
    main()
