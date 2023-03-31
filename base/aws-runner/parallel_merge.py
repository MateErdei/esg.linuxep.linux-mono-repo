#!/usr/bin/env python
# Copyright 2023 Sophos Limited. All rights reserved.

import argparse
import glob
import os
import queue
import subprocess
import sys
import time
import threading

RESULT = 0

DELETE = os.environ.get("NO_DELETE", "0") != "1"


def run_rebot(platform: str):
    """

      python3 -m robot.rebot --merge -o ./results/${PLATFORM}-output.xml \
          -l ./results/${PLATFORM}-log.html -r ./results/${PLATFORM}-report.html \
          -N ${PLATFORM}  ./results-combine-workspace/${PLATFORM}*
      rm -rf ./results-combine-workspace/${PLATFORM}*

    :param platform:
    :return:
    """
    global RESULT
    src_glob = f"./results-combine-workspace/{platform}*"
    srcs = glob.glob(src_glob)
    if len(srcs) == 0:
        RESULT += 1
        print("Failed to match anything with " + src_glob, file=sys.stderr)
        return

    command = ["python3", "-m", "robot.rebot", "--merge",
               "-o", f"./results/{platform}-output.xml",
               "-l", f"./results/{platform}-log.html",
               "-r", f"./results/{platform}-report.html",
               "-N", platform]
    command += srcs
    print("Running "+" ".join(command))
    result = subprocess.run(command)
    if result.returncode != 0:
        print("robot.rebot failed!", result.returncode, file=sys.stderr)
        RESULT += 1

    if DELETE:
        for src in srcs:
            os.unlink(src)


def run_queue(q):
    try:
        while True:
            platform = q.get(False)
            print("Processing", platform)
            run_rebot(platform)
    except queue.Empty:
        return


def main(argv):
    global RESULT
    start = time.time()
    parser = argparse.ArgumentParser(
        prog='parallel_merge.py',
        description='Runs robot rebot in mulitple threads',
        epilog='Part of aws-runner')
    parser.add_argument("platforms", nargs="+")
    parser.add_argument("-j", type=int, action="store", default=1)
    args = parser.parse_args(argv[1:])

    q = queue.Queue()
    for platform in args.platforms:
        q.put(platform)

    assert q.qsize() == len(args.platforms)

    threads = []
    for i in range(args.j):
        t = threading.Thread(target=run_queue, args=[q])
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

    assert q.empty()

    end = time.time()
    duration = end - start
    print("Completed parallel_merge in %f seconds with %d threads" % (duration, args.j))
    return RESULT


if __name__ == "__main__":
    sys.exit(main(sys.argv))
