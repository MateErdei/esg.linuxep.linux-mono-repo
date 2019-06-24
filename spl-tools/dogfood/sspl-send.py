import sys
if sys.version_info[0] < 3:
    raise Exception("Must be using Python 3")

import time
import socket
import os
import re
from threading import Thread

try:
    import mysql.connector
except:
    print("No module named 'mysql', please install it: python3 -m pip install mysql-connector-python")
    exit()


def follow(thefile):
    thefile.seek(0, 2) # Go to the end of the file
    while True:
        try:
            line = thefile.readline()
        except:
            print("could not read line")
        if not line:
            time.sleep(0.1) # Sleep briefly
            continue
        yield line


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

    if len(line) > 1000:
        return

    if line == "":
        return

    cursor = db.cursor()
    time = get_time_string_from_log_line(line)

    # if time is "" then the log line probably wasn't logged as a single line and thus did not have a timestamp in it.
    # So append this line to last committed line.
    if time == "":
        if last_id is None:
            # if here then we've started capturing logs half way through a multi line log write
            # just throw the line away until we get the start of complete log line with a timestamp in it.
            return None
        df_sql = "UPDATE dogfood_logs SET log_msg = CONCAT(log_msg, '\n', '{}') WHERE idlogs = {}".format(line, last_id)
        cursor.execute(df_sql)
        db.commit()
        print("Updated multiline log row with id:{}".format(last_id))

    else:
        df_sql = "INSERT INTO dogfood_logs (log_time, log_msg, log_path, log_name, hostname, ip) VALUES (%s, %s, %s, %s, %s, %s)"
        df_val = (time, line, log_path, os.path.basename(log_path), hostname, ip)
        cursor.execute(df_sql, df_val)
        db.commit()
        last_id = cursor.lastrowid

    print("inserted, id:{}".format(last_id))
    return last_id


def get_ip():
    try:
        import socket
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return ""


def monitor_log_file(log_path):
    hostname = socket.gethostname()
    ip = get_ip()

    # This account has only insert privileges (not even select) so it is safe to include here.
    # Dashboard sanitizes html so a script being inserted for example will not do anything when displayed.
    dogfood_db = mysql.connector.connect(
        host="10.55.36.229",
        user="inserter",
        passwd="insert",
        database="dogfood"
    )

    last_log_id = None
    while True:
        try:
            with open(log_path, "r") as f:
                for line in follow(f):
                    last_log_id = send_log_line_to_db(line, log_path, dogfood_db, ip, hostname, last_log_id)
            print("Monitor died:{}".format(log_path))
            print("Restarting it...")
        except:
            print("Monitor died and threw:{}".format(log_path))
            print("Restarting it...")
            pass


g_product = "SSPL"

def main():

    log_dirs = ["/opt/sophos-spl"]

    from pathlib import Path
    logs = []
    for log_dir in log_dirs:
        for filename in Path(log_dir).glob('**/*.log'):
            logs.append(str(filename))

    threads = []
    for log in logs:
        print("Monitoring Log: {}".format(log))
        thread = Thread(target=monitor_log_file, args=[log])
        threads.append(thread)

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    print("threads finished...exiting")


if __name__ == "__main__":
    main()
