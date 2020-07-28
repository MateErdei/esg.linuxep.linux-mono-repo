#!/usr/bin/env python3.7

import argparse
import json
import sys
import time
import subprocess
import os
import glob
# import logging

try:
    from performance.websocket_server import LTserver, certificates
except:
    from websocket_server import LTserver, certificates

# log = logging.getLogger('RunLocalLiveTerminal')

# todo consider moving this to go through Management agent - for now trigger it directly to get it working
# <?xml version="1.0"?><ns:commands xmlns:ns="http://www.sophos.com/xml/mcs/commands" schemaVersion="1.0"><command><id>ABC</id><seq>1</seq><appId>LiveQuery</appId><creationTime>2020-06-09T15:30:08.000Z</creationTime><ttl>PT10000S</ttl><body>{"type": "sophos.mgt.action.RunLiveQuery", "name": "users", "query": "SELECT * from users"}</body></command></ns:commands>
def trigger_endpoint_terminal(path: str):
    # log.info("Triggering terminal on path: {}".format(path))
    path_to_write_file = "/opt/sophos-spl/plugins/liveresponse/var/{}".format(path)

    trigger = {"url": "https://localhost:4443/{}".format(path),
               "thumbprint": "2d03c43bfc9ebe133e0c22df61b840f94b3a3bd5b05d1d715cc92b2debcb6f9d",
               "endpoint_id": "endpointid",
               "customer_id": "customerid"
               }
    with open(path_to_write_file, 'w') as trigger_file:
        trigger_file.write(json.dumps(trigger))
    subprocess.Popen(["/opt/sophos-spl/plugins/liveresponse/bin/sophos-live-terminal", "-i", path])


def create_server_and_send_string(string_to_send: str, number_of_parallel_terminals: int, keepalive: int):
    paths = []
    for n in range(0, number_of_parallel_terminals):
        paths.append("path{}".format(n))
    # log.info(paths)
    with LTserver.LTServer('localhost', 4443, certificates.CERTIFICATE_PEM) as server:
        for path in paths:
            path_with_slash = "/{}".format(path)
            trigger_endpoint_terminal(path)
            print("Checking terminal connected to path: {}".format(path))
            while not server.match_message('root@', path_with_slash):
                time.sleep(1)
            # log.info("Path connected: {}".format(path))

        # For each terminal, which is running on its own path, have a file which is written by the command itself to
        # prove that the string has been sent and also saved to a file, this means we know the command has finished and
        # have proven that it was successful. We need to remove all previous evidence files first.
        evidence_file_prefix = "/tmp/evidence-"
        file_list = glob.glob("{}*".format(evidence_file_prefix))
        for filePath in file_list:
            try:
                os.remove(filePath)
            except:
                pass
                # log.info("Error while deleting file : ", filePath)

        # log.info("Sending, path: {}, string: {}".format(path, string_to_send))
        timeout = time.time() + 100
        start_time = get_current_unix_epoch_in_seconds()
        # Write the string to disk on the endpoint so that this script can verify it was received and saved
        # So the command will be: echo -n "string to send" > /some/file/path/logged-message
        for path in paths:
            evidence_file_path = "{}{}".format(evidence_file_prefix, path)
            server.send_message_with_newline("echo -n '{}' > {}".format(string_to_send, evidence_file_path), path_with_slash)
            saved_msg = None
            while string_to_send != saved_msg and time.time() < timeout:
                try:
                    with open(evidence_file_path, 'r') as evidence_file:
                        saved_msg = evidence_file.read()
                except:
                    # just keep trying until timeout.
                    pass
        if keepalive != 0:
            time.sleep(keepalive)

        end_time = get_current_unix_epoch_in_seconds()
        return start_time, end_time


def get_current_unix_epoch_in_seconds():
    return time.time()


def add_options():
    parser = argparse.ArgumentParser(description='Runs Local Live Response Terminals')
    parser.add_argument('-f', '--file', required=True, action='store', help="File path with the string contents that you want to send")
    parser.add_argument('-n', '--number-of-terminals', default=1, action='store', help="Number of terminals to run in parallel, each sending the same content")
    parser.add_argument('-k', '--keepalive', default=0, action='store', help="Keep the terminal session alive for this many seconds before closing")
    return parser


def main():
    parser = add_options()
    args = parser.parse_args()

    with open(args.file, 'r') as file_to_read:
        string_to_send = file_to_read.read()

    certificates.InstallCertificates.install_certificate()

    start_time, end_time = create_server_and_send_string(string_to_send, int(args.number_of_terminals), int(args.keepalive))

    result = {"start_time": start_time,
              "end_time": end_time,
              "duration": end_time - start_time,
              "success": (end_time - start_time) < 60}
    # "success" in this case is to make sure it has not timed out, it keeps trying until it has succeed or timed out.
    # If this took longer than a minute then something is broken, so fail.

    print("RESULTS:{}".format(json.dumps(result)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
