# Copyright 2019-2021 Sophos Limited. All rights reserved.
"""
Wrapper script to allow SDDS3 tools to invoke digest_sign from bazel.
Takes care of setting environment variables that need to be defined before
calling the tool, then passes arguments on to the appropriate tool to
do the signing.
"""

import os
import json
import subprocess
import sys


def _set_environment():
    # Set the path so the tool can be found. The path varies between the CI
    # environment and local dev machines.

    # needed for linux?
    os.environ["PATH"] += r":/home/pair/gitrepos/sspl-tools/tap_venv/bin"

    # If the environment file isn't there (for example, because the user is
    # running bazel commands directly), just silently continue.
    if not os.path.exists('./tools/environment.json'):
        return

    # Set the specific environment variables that the signing tools care about.
    # These variables won't exist on a developer machine.
    environment = json.loads(open('./tools/environment.json').read())
    if "CI_SIGNING_URL" in environment:
        os.environ["CI_SIGNING_URL"] = environment["CI_SIGNING_URL"]
    if "BUILD_JWT_PATH" in environment:
        os.environ["BUILD_JWT_PATH"] = environment["BUILD_JWT_PATH"]
    if "VIRTUAL_ENV" in environment:
        os.environ["VIRTUAL_ENV"] = environment["VIRTUAL_ENV"]


def main(args):
    if len(args) < 2:
        raise Exception("ERROR: signing_tools requires at least one argument, which is the name of the tool to invoke")

    _set_environment()

    # Usage: env.py <*.exe or *.py> [args]

    cmd = args[1]
    _, ext = os.path.splitext(cmd)

    if ext == ".py":
        subprocess.check_call([sys.executable] + args[1:])
    else:
        subprocess.check_call(args[1:])


if __name__ == "__main__":
    main(sys.argv)
