import json
import os
import shutil
import tarfile

import robot
import sys


def copy_supplements(src, dest):
    shutil.copytree(os.path.join(src, "supplement"), os.path.join(dest, "supplement"))

    src_package_path = os.path.join(src, "package")
    dest_package_path = os.path.join(dest, "package")
    for (dir_path, dir_names, filenames) in os.walk(src_package_path):
        for package in filenames:
            if package.startswith(("DataSetA", "LocalRepData", "ML_MODEL3_LINUX_X86_64",
                                   "RuntimeDetectionRules", "ScheduledQueryPack", "SSPLFLAGS")):
                if not os.path.isfile(os.path.join(dest_package_path, package)):
                    print(f"Copying Package: {package} from {src_package_path} to {dest_package_path}")
                    shutil.copy(os.path.join(src_package_path, package), os.path.join(dest_package_path, package))


def main(argv):
    exclude = argv
    tags = {"include": ["PACKAGE"], "exclude": exclude}

    os.environ["INPUT_DIRECTORY"] = "/opt/test/inputs"
    os.environ["TEST_SCRIPT_PATH"] = fr"{os.environ['INPUT_DIRECTORY']}/test_scripts/"

    robot_args = {
        "path":  os.environ["TEST_SCRIPT_PATH"],
        "name": "SSPL_warehouse",
        "loglevel": "DEBUG",
        "consolecolors": "ansi",
        "include": tags["include"],
        "exclude": tags["exclude"],
        "log": "/opt/test/logs/log.html",
        "output": "/opt/test/logs/output.xml",
        "report": "/opt/test/logs/report.html",
        "suite": "*"
    }

    try:
        os.environ["BASE_TEST_UTILS"] = os.path.join(os.environ["INPUT_DIRECTORY"], "base_test_utils")
        tar = tarfile.open(os.path.join(os.environ["BASE_TEST_UTILS"], "SystemProductTestOutput.tar.gz"))
        tar.extractall(os.environ["BASE_TEST_UTILS"])
        tar.close()

        unpacked_utils = os.path.join(os.environ["BASE_TEST_UTILS"], "SystemProductTestOutput", "testUtils")
        shutil.move(os.path.join(unpacked_utils, "libs"), os.environ["TEST_SCRIPT_PATH"])
        shutil.move(os.path.join(unpacked_utils, "SupportFiles"), os.environ["TEST_SCRIPT_PATH"])
    except Exception as ex:
        print(f"Failed to unpack testUtils from base: {ex}")
    os.environ["SUPPORT_FILES"] = os.path.join(os.environ["TEST_SCRIPT_PATH"], "SupportFiles")
    os.environ["LIB_FILES"] = os.path.join(os.environ["TEST_SCRIPT_PATH"], "libs")

    # TODO LINUXDAR-7079 supplements are not published for release branches
    try:
        os.environ["VUT_WAREHOUSE_REPO_ROOT"] = os.path.join(os.environ["INPUT_DIRECTORY"], "repo")

        os.environ["DOGFOOD_WAREHOUSE_REPO_ROOT"] = os.path.join(os.environ["INPUT_DIRECTORY"], "dogfood_repo")
        os.environ["CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT"] = os.path.join(os.environ["INPUT_DIRECTORY"], "current_shipping_repo")

        copy_supplements(os.environ["VUT_WAREHOUSE_REPO_ROOT"], os.environ["DOGFOOD_WAREHOUSE_REPO_ROOT"])
        copy_supplements(os.environ["VUT_WAREHOUSE_REPO_ROOT"], os.environ["CURRENT_SHIPPING_WAREHOUSE_REPO_ROOT"])
    except Exception as ex:
        print(f"Failed to copy supplements for dogfood/current_shipping: {ex}")

    try:
        # Create the TAP Robot result listener.
        from pubtap.robotframework.tap_result_listener import tap_result_listener
        listener = tap_result_listener(robot_args["path"], tags, robot_args["name"])
        robot_args["listener"] = listener
    except ImportError:
        print("pubtap not available")
    except json.decoder.JSONDecodeError:
        # When running locally you can not initialise the TAP Results Listener
        pass

    return robot.run(robot_args["path"], **robot_args)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
