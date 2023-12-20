# Copyright 2020-2023 Sophos Limited. All rights reserved.
import argparse
from dirsync import sync
import distro
import json
import os
import shutil

import robot
import sys


def copy_supplements(src, dest):
    shutil.rmtree(os.path.join(dest, "supplement"), ignore_errors=True)
    shutil.copytree(os.path.join(src, "supplement"), os.path.join(dest, "supplement"))

    src_package_path = os.path.join(src, "package")
    dest_package_path = os.path.join(dest, "package")
    for (dir_path, dir_names, filenames) in os.walk(src_package_path):
        for package in filenames:
            if package.startswith(("DataSetA", "LocalRepData", "ML_MODEL3_LINUX_X86_64", "ML_MODEL3_LINUX_ARM64",
                                   "RuntimeDetectionRules", "ScheduledQueryPack", "SSPLFLAGS")):
                if not os.path.isfile(os.path.join(dest_package_path, package)):
                    print(f"Copying Package: {package} from {src_package_path} to {dest_package_path}")
                    shutil.copy(os.path.join(src_package_path, package), os.path.join(dest_package_path, package))


def get_platform_exclusions():
    id = distro.id()
    pretty_name = distro.name(pretty=True)
    print(f"Pretty name to be matched for exclusions: {pretty_name}")
    exclusions = []
    if id == "amazon":
        exclusions.append("EXCLUDE_AMAZON")
        if pretty_name.startswith("Amazon Linux 2023"):
            exclusions.append("EXCLUDE_AMAZON_LINUX2023")
        elif pretty_name.startswith("Amazon Linux 2"):
            exclusions.append("EXCLUDE_AMAZON_LINUX2")
    elif id == "centos":
        exclusions.append("EXCLUDE_CENTOS")
    elif id == "debian":
        exclusions.append("EXCLUDE_DEBIAN")
    elif id == "rhel":
        exclusions.append("EXCLUDE_RHEL")
    elif id == "sles":
        exclusions.append("EXCLUDE_SLES")
        if pretty_name.startswith("SUSE Linux Enterprise Server 12"):
            exclusions.append("EXCLUDE_SLES12")
        elif pretty_name.startswith("SUSE Linux Enterprise Server 15"):
            exclusions.append("EXCLUDE_SLES15")
    elif id == "ubuntu":
        exclusions.append("EXCLUDE_UBUNTU")
        if pretty_name.startswith("Ubuntu 18.04"):
            exclusions.append("EXCLUDE_UBUNTU18")
        elif pretty_name.startswith("Ubuntu 20.04"):
            exclusions.append("EXCLUDE_UBUNTU20")
        elif pretty_name.startswith("Ubuntu 22.04"):
            exclusions.append("EXCLUDE_UBUNTU22")
    else:
        print(f"Unhandled id {id}. Please extend platform exclusions to handle it")
    return exclusions


class ExtendAction(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        items = getattr(namespace, self.dest) or []
        items.extend(values)
        setattr(namespace, self.dest, items)


def main(argv):
    parser = argparse.ArgumentParser()
    parser.register('action', 'extend', ExtendAction)
    parser.add_argument('--include', nargs='+', help='keywords to include', action="extend")
    parser.add_argument('--exclude', nargs='+', help='keywords to exclude', action="extend")
    parser.add_argument("--test", "--TEST", help="single test to run", default=os.environ.get('TEST', None))
    parser.add_argument("--suite", "--SUITE", help="single test suite to run", default=os.environ.get('SUITE', None))
    parser.add_argument("--debug", "--DEBUG", action="store_true", default=os.environ.get('DEBUG', False))
    parser.add_argument('--fixed-versions', nargs='+', help='SPL FTS Releases', action="extend")
    args = parser.parse_args()

    tags = {'include': [], 'exclude': []}
    if args.include is not None:
        tags['include'] = args.include

    tags['exclude'] = get_platform_exclusions()
    if args.exclude is not None:
        tags['exclude'].extend(args.exclude)

    if args.fixed_versions is not None:
        os.environ["FIXED_VERSIONS"] = ','.join(args.fixed_versions)
    else:
        tags["exclude"].append("FIXED_VERSIONS")

    INPUT_DIRECTORY = "/opt/test/inputs"
    os.environ["INPUT_DIRECTORY"] = INPUT_DIRECTORY
    os.environ["TEST_SCRIPT_PATH"] = f"{INPUT_DIRECTORY}/test_scripts/"

    path = os.environ["TEST_SCRIPT_PATH"]
    robot_args = {
        "name": "SSPL_warehouse",
        "loglevel": "DEBUG",
        "consolecolors": "ansi",
        "include": tags["include"],
        "exclude": tags["exclude"],
        "log": "/opt/test/logs/log.html",
        "output": "/opt/test/logs/output.xml",
        "report": "/opt/test/logs/report.html",
        "variable": [
            f"COMMON_TEST_LIBS:{os.path.join(INPUT_DIRECTORY, 'common_test_libs')}",
            f"RTD_FAILING:{'1' if 'RTD_CHECKED' in args.exclude else '0'}",
        ],
    }

    if os.path.isfile("/tmp/BullseyeCoverageEnv.txt"):
        # Disable failing the test run if coverage is enabled
        robot_args["statusrc"] = False

    if args.test:
        robot_args['test'] = args.test
    if args.suite:
        robot_args['suite'] = args.suite

    os.environ["SUPPORT_FILES"] = os.path.join(INPUT_DIRECTORY, "SupportFiles")
    os.environ["LIB_FILES"] = os.path.join(os.environ["TEST_SCRIPT_PATH"], "libs")
    os.environ['BASE_DIST'] = os.path.join(INPUT_DIRECTORY, "base")
    os.environ['SSPL_ANTI_VIRUS_PLUGIN_SDDS'] = os.path.join(INPUT_DIRECTORY, "av")
    os.environ['SSPL_EDR_PLUGIN_SDDS'] = os.path.join(INPUT_DIRECTORY, "edr")
    os.environ['SSPL_DEVICE_ISOLATION_PLUGIN_SDDS'] = os.path.join(INPUT_DIRECTORY, "di")
    os.environ['SSPL_EVENT_JOURNALER_PLUGIN_SDDS'] = os.path.join(INPUT_DIRECTORY, "ej")
    os.environ['SSPL_LIVERESPONSE_PLUGIN_SDDS'] = os.path.join(INPUT_DIRECTORY, "lr")
    os.environ['SSPL_RUNTIMEDETECTIONS_PLUGIN_SDDS'] = os.path.join(INPUT_DIRECTORY, "rtd")
    os.environ['SSPL_RA_PLUGIN_SDDS'] = os.path.join(INPUT_DIRECTORY, "ra")
    os.environ['THIN_INSTALLER_OVERRIDE'] = os.path.join(INPUT_DIRECTORY, "thin_installer")
    os.environ['WEBSOCKET_SERVER'] = os.path.join(INPUT_DIRECTORY, "websocket_server")
    os.environ['LT_UTILS'] = os.path.join(INPUT_DIRECTORY, "liveterminal_test_scripts")
    os.environ['SYSTEMPRODUCT_TEST_INPUT'] = INPUT_DIRECTORY
    os.environ['SSPL_EVENT_JOURNALER_PLUGIN_MANUAL_TOOLS'] = os.path.join(INPUT_DIRECTORY, "ej_manual_tools", "JournalReader")
    shutil.rmtree(os.path.join(os.environ['SSPL_RUNTIMEDETECTIONS_PLUGIN_SDDS'], "content_rules"), ignore_errors=True)
    shutil.copytree(os.path.join(INPUT_DIRECTORY, "rtd_content_rules"), os.path.join(os.environ['SSPL_RUNTIMEDETECTIONS_PLUGIN_SDDS'], "content_rules"))
    os.environ["SDDS3_BUILDER"] = os.path.join(os.environ["INPUT_DIRECTORY"], "sdds3", "sdds3-builder")

    # TODO LINUXDAR-7079 supplements are not published for release branches
    try:
        os.environ["VUT_WAREHOUSE_ROOT"] = os.path.join(INPUT_DIRECTORY, "repo")
        os.environ["VUT_WAREHOUSE_ROOT_999"] = os.path.join(INPUT_DIRECTORY, "repo999")
        sync(os.environ["VUT_WAREHOUSE_ROOT_999"], os.environ["VUT_WAREHOUSE_ROOT"], "sync")

        os.environ["DOGFOOD_WAREHOUSE_REPO_ROOT"] = os.path.join(INPUT_DIRECTORY, "dogfood_repo")
        os.environ["CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT"] = os.path.join(INPUT_DIRECTORY, "current_shipping_repo")


        # if doing prod build, copying dev supplements that have been pulled in to current prod build directory
        if os.path.isdir(os.path.join(INPUT_DIRECTORY, "dev_sdds3")):
            copy_supplements(os.path.join(INPUT_DIRECTORY, "dev_sdds3"), os.environ["VUT_WAREHOUSE_ROOT"])

        copy_supplements(os.environ["VUT_WAREHOUSE_ROOT"], os.environ["DOGFOOD_WAREHOUSE_REPO_ROOT"])
        copy_supplements(os.environ["VUT_WAREHOUSE_ROOT"], os.environ["CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT"])

    except Exception as ex:
        print(f"Failed to copy supplements for dogfood/current_shipping: {ex}")

    try:
        # Create the TAP Robot result listener.
        from pubtap.robotframework.tap_result_listener import tap_result_listener
        listener = tap_result_listener(path, tags, robot_args["name"])
        robot_args["listener"] = listener
    except ImportError:
        print("pubtap not available")
    except json.decoder.JSONDecodeError:
        # When running locally you can not initialise the TAP Results Listener
        pass

    sys.path.append(os.path.join(INPUT_DIRECTORY, 'common_test_libs'))
    return robot.run(path, **robot_args)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
