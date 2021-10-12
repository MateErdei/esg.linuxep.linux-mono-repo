#!/usr/bin/env python3.7

import argparse
import glob
import json
import os
import subprocess
import sys
import time

from websocket_server import LTserver, certificates

from performance.PerformanceResources import get_current_unix_epoch_in_seconds


def trigger_endpoint_terminal(path: str):
    path_to_write_file = f"/opt/sophos-spl/plugins/liveresponse/var/{path}"

    trigger = {"url": f"https://localhost:4443/{path}",
               "thumbprint": "2d03c43bfc9ebe133e0c22df61b840f94b3a3bd5b05d1d715cc92b2debcb6f9d",
               "endpoint_id": "endpointid",
               "customer_id": "customerid",
               "jwt_token": "jwt-token",
               "device_id": "deviceid",
               "tenant_id": "tenantid",
               "user_agent": "useragent"
               }
    with open(path_to_write_file, 'w') as trigger_file:
        trigger_file.write(json.dumps(trigger))
    subprocess.Popen(["/opt/sophos-spl/plugins/liveresponse/bin/sophos-live-terminal", "-i", path])


def create_server_and_send_string(string_to_send: str, number_of_parallel_terminals: int, keepalive: int):
    paths = []
    for n in range(0, number_of_parallel_terminals):
        paths.append(f"path{n}")
    connected = False
    with LTserver.LTServer('localhost', 4443, certificates.CERTIFICATE_PEM) as server:
        for path in paths:
            path_with_slash = f"/{path}"
            trigger_endpoint_terminal(path)
            print(f"Checking terminal connected to path: {path}")
            connected = server.match_message('root@', path_with_slash, 60)

        if not connected:
            print("Endpoint not connected to fake terminal server")
            exit(1)

        # For each terminal, which is running on its own path, have a file which is written by the command itself to
        # prove that the string has been sent and also saved to a file, this means we know the command has finished and
        # have proven that it was successful. We need to remove all previous evidence files first.
        evidence_file_prefix = "/tmp/evidence-"
        file_list = glob.glob(f"{evidence_file_prefix}*")
        for filePath in file_list:
            try:
                os.remove(filePath)
            except:
                pass

        timeout = time.time() + 100
        start_time = get_current_unix_epoch_in_seconds()
        # Write the string to disk on the endpoint so that this script can verify it was received and saved
        # So the command will be: echo -n "string to send" > /some/file/path/logged-message
        for path in paths:
            evidence_file_path = f"{evidence_file_prefix}{path}"
            server.send_message_with_newline(f"echo -n '{string_to_send}' > {evidence_file_path}", path_with_slash)
            saved_msg = None
            while string_to_send != saved_msg and time.time() < timeout:
                try:
                    with open(evidence_file_path, 'r') as evidence_file:
                        saved_msg = evidence_file.read()
                except:
                    # just keep trying until timeout.
                    pass
        if keepalive != 0:
            print("Running keep alive section of test")
            # If this is the keep alive test we want to check the terminal is still running ok every second up until
            # the keep alive timeout is reached.
            target_time = start_time + keepalive
            while target_time > get_current_unix_epoch_in_seconds():
                # Use the current unix timestamp as a marker message, it will be unique so we don't see old messages.
                marker_message = str(get_current_unix_epoch_in_seconds())
                server.send_message_with_newline(f"echo {marker_message}", path_with_slash)
                if not server.match_message(marker_message, path_with_slash):
                    print(f"Test failed! could not find the message: {marker_message}")
                    break
                time.sleep(1)

        end_time = get_current_unix_epoch_in_seconds()
        return start_time, end_time


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

    # Install certs needed for local LTserver
    certificates.InstallCertificates.install_certificate()

    start_time, end_time = create_server_and_send_string(string_to_send, int(args.number_of_terminals), int(args.keepalive))

    result = {"start_time": start_time,
              "end_time": end_time,
              "duration": end_time - start_time,
              "success": True}

    print(f"RESULTS:{json.dumps(result)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
