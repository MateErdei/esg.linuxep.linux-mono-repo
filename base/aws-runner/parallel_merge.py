#!/usr/bin/env python
# Copyright 2023 Sophos Limited. All rights reserved.

import argparse
import glob
import os
import subprocess
import sys
import time


def run_rebot(platform: str):
    """

      python3 -m robot.rebot --merge -o ./results/${PLATFORM}-output.xml \
          -l ./results/${PLATFORM}-log.html -r ./results/${PLATFORM}-report.html \
          -N ${PLATFORM}  ./results-combine-workspace/${PLATFORM}*
      rm -rf ./results-combine-workspace/${PLATFORM}*

    :param platform:
    :return:
    """
    srcs = glob.glob(f"./results-combine-workspace/{platform}*")
    command = ["python", "-m", "robot.rebot", "--merge",
               "-o", f"./results/{platform}-output.xml",
               "-l", f"./results/{platform}-log.html",
               "-r", f"./results/{platform}-report.html",
               "-N", platform]
    command += srcs
    subprocess.run(command)

    for src in srcs:
        os.unlink(src)


def main(argv):
    parser = argparse.ArgumentParser(
        prog='parallel_merge.py',
        description='Runs robot rebot in mulitple threads',
        epilog='Part of aws-runner')
    parser.add_argument("platforms", nargs="+")
    parser.add_argument("-j", type=int, action="store")

    args = parser.parse_args(argv[1:])
    for platform in args.platforms:
        run_rebot(platform)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
