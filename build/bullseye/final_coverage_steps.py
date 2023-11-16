# Copyright 2023 Sophos Limited. All rights reserved.

import os
import os.path
import subprocess
import sys


def bullseye_test_coverage(branch):
    coverage_results_dir = "/opt/test/coverage"
    test_coverage_json_path = os.path.join(coverage_results_dir, "test_coverage.json")
    covfile_path = os.path.join(coverage_results_dir, "all", "monorepo.cov")

    test_coverage_args = [
        "python3",
        "-u",
        "/opt/test/inputs/bazel_tools/tools/src/bullseye/test_coverage.py",
        "--output",
        test_coverage_json_path,
        covfile_path,
    ]
    if branch == "develop":
        test_coverage_args += ["--upload", "--upload-job", "UnifiedPipelines/linuxep/linux-mono-repo"]
    subprocess.check_call(test_coverage_args)


def main(argv):
    branch = argv[1]

    # Dict from group name to list of COVFILEs to merge for that group
    coverage_groupings = {
        "unit": ["/opt/test/inputs/coverage_unit/monorepo_unit.cov"],
        "all": ["/opt/test/inputs/coverage_unit/monorepo_unit.cov"],
    }

    for dir_entry in os.scandir("/opt/test/inputs/coverage"):
        # Example: /opt/test/inputs/coverage/linux_mono_repo.products.testing.av_tests.integration_TAP_PARALLEL1_linux_x64_bullseye_centos7_x64_aws_server_en_us/machine/monorepo.cov

        if not dir_entry.is_dir():
            continue

        covfile = os.path.join(dir_entry.path, "machine", "monorepo.cov")

        component, test = dir_entry.name.split(".")[3:]
        component = "_".join(component.split("_")[:-1])
        test = test.split("_")[0]

        group = component

        # If the test name starts with something other than TAP(_PARALLELx) or linux(_x64_bullseye), that means it's a
        # subset of the tests under that component, so group them separately
        if test != "TAP" and test != "linux":
            group += "_" + test

        group_covfiles = coverage_groupings.setdefault(group, [])
        group_covfiles.append(covfile)
        coverage_groupings["all"].append(covfile)

    # Merge COVFILEs for each group, generate HTMLs and upload them to Allegro
    for group, group_covfiles in coverage_groupings.items():
        subprocess.check_call(
            ["bash", "/opt/test/inputs/bullseye_scripts/merge_and_upload.sh", branch, group, *group_covfiles]
        )

    # Generate test_coverage.json and possibly submit it to CI
    bullseye_test_coverage(branch)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
