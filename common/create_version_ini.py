# Copyright 2023 Sophos Limited. All rights reserved.

import sys
import datetime


def main(argv):
    component_name, component_version_file, output_file = argv[1:]

    with open(component_version_file) as f:
        version = f.read().strip()

    build_date = datetime.datetime.utcnow().strftime("%Y-%m-%d")

    with open("/build/COMMIT_HASH") as f:
        commit_hash = f.read().strip()

    with open(output_file, "w") as f:
        f.write(
            f"""PRODUCT_NAME = {component_name}
PRODUCT_VERSION = {version}
BUILD_DATE = {build_date}
COMMIT_HASH = {commit_hash}
"""
        )


if __name__ == "__main__":
    main(sys.argv)
