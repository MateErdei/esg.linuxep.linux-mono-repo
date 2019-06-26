import sys
if sys.version_info[0] < 3:
    raise Exception("Must be using Python 3")

import time
import os
import re
from pathlib import Path
import datetime
import shutil


try:
    import mysql.connector
except:
    print("No module named 'mysql', please install it: python3 -m pip install mysql-connector-python")
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
        pattern = "[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9]\.[0-9][0-9][0-9]"
        reg = re.compile(pattern)
        try:
            t = reg.findall(line)[0]
        except:
            print("Could not find date in:{}".format(line))
            return ""
        t = t.replace('T', ' ')
        t = t.split('.')[0]
    return t


def send_log_line_to_db(line, log_path, db, ip, hostname, last_id):
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
            return None

        df_sql = "UPDATE dogfood_logs SET log_msg = CONCAT(log_msg, '\n', %s) WHERE idlogs = %s"
        cursor.execute(df_sql, (line, last_id))
        db.commit()

    else:
        df_sql = "INSERT INTO dogfood_logs (log_time, log_msg, log_path, log_name, hostname, ip) VALUES (%s, %s, %s, %s, %s, %s)"
        df_val = (extracted_time, line, "diagnose", os.path.basename(log_path), hostname, ip)
        cursor.execute(df_sql, df_val)
        db.commit()
        last_id = cursor.lastrowid

    print("inserted, id:{}".format(last_id))
    return last_id


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


def extract_hostname(extracted_tar_path):
    with open(os.path.join(extracted_tar_path, "SystemFiles", "hostnamectl"), 'r') as hostnamectl:
        for line in hostnamectl:
            if "hostname:" in line:
                x = line.split(":")
                if len(x) > 1:
                    return x[1].strip()


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


def process_diagnose_file(tar_path):
    global g_extract_dir

    print(tar_path)

    if not os.path.isdir(g_extract_dir):
        os.mkdir(g_extract_dir)
    tar_file_name = os.path.basename(tar_path)
    local_tar_path = os.path.join(g_extract_dir, tar_file_name)
    shutil.copyfile(tar_path, local_tar_path)

    sub_dir = os.path.join(g_extract_dir,tar_file_name + ".dir")

    import tarfile
    tar = tarfile.open(local_tar_path)
    tar.extractall(sub_dir)
    tar.close()

    logs = []
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

    hostname = extract_hostname(sub_dir)
    ip = extract_ip(sub_dir)
    print("Hostname: {}".format(hostname))
    print("IP: {}".format(ip))


    # This account has only insert privileges (not even select) so it is safe to include here.
    # Dashboard sanitizes html so a script being inserted for example will not do anything when displayed.
    dogfood_db = mysql.connector.connect(
        host="10.55.36.229",
        user="inserter",
        passwd="insert",
        database="dogfood"
    )

    for log in logs:
        process_log_file(hostname, dogfood_db, ip, log)

    mark_tar_as_processed(tar_path)


def process_log_file(hostname, db, ip, log_file_path):
    print("Processing log:{}".format(log_file_path))
    last_id = None
    with open(log_file_path, 'r') as log:
        for line in log:
            last_id = send_log_line_to_db(line, log_file_path, db, ip, hostname, last_id)
            time.sleep(0.1)


# Globals
g_product = "SSPL"
g_extract_dir = "/tmp/dogfood-extract"


def main():

    global g_product

    if len(sys.argv) != 3:
        usage()
        exit(1)

    product = sys.argv[1].lower()
    if "sav" in product:
        g_product = "SAV"
    #elif "sspl" in arg:
    #    g_product = "SSPL"

    print("Product: {}".format(g_product))


    completed_tars = []
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
            print("The file: {}, does not start with sspl-diagnose and end with .tar.gz, so assuming it is not an SSPL dog food tar.gx file.",format(tar_path))
    else:
        exit(1)

    tars_to_process = [x for x in tars if not tar_already_processed(x)]

    skip_list = set(tars) - set(tars_to_process)
    for skipped_tar in skip_list:
        print("Skipping because already processed: {}".format(skipped_tar))

    for tar in tars_to_process:
        process_diagnose_file(tar)

    if g_extract_dir.startswith("/tmp/") and os.path.exists(g_extract_dir):
        shutil.rmtree(g_extract_dir)


if __name__ == "__main__":
    main()
