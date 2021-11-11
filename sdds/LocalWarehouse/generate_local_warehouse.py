#!/usr/bin/env python3
import argparse
import os
import logging
import subprocess
import shutil
import time
import zipfile

logging.basicConfig()
# import build_scripts


LOGGER = logging.getLogger(__name__)

SCRIPT_NAME = os.path.basename(__file__)
SCRIPT_PATH = os.path.abspath(__file__)
SCRIPT_DIR = os.path.dirname(SCRIPT_PATH)
DEFAULT_WORKINGDIR = os.path.join(SCRIPT_DIR)

VUT_CUSTOMER_DEST = "local_warehouses/dev/sspl-warehouse/vut/warehouse/customer"
VUT_WAREHOUSE_DEST = "local_warehouses/dev/sspl-warehouse/develop/warehouse/warehouse"
NNN_CUSTOMER_DEST = "local_warehouses/dev/sspl-warehouse/edr-mdr-999/warehouse/customer"
NNN_WAREHOUSE_DEST = "local_warehouses/dev/sspl-warehouse/edr-mdr-999/warehouse/warehouse"
SUPPORT_FILES_DEST = "./utils"
package_xml = \
f"""<?xml version="1.0" encoding="utf-8"?>
<package name="system-product-tests">
    <inputs>
        <workingdir>{{}}</workingdir>
        <build-asset project="linuxep" repo="sspl-warehouse">
            <development-version branch="{{}}" />
            <include artifact-path="develop/customer" dest-dir="{VUT_CUSTOMER_DEST}" />
            <include artifact-path="develop/warehouse" dest-dir="{VUT_WAREHOUSE_DEST}" />
            <include artifact-path="edr-mdr-999/customer" dest-dir="{NNN_CUSTOMER_DEST}" />
            <include artifact-path="edr-mdr-999/warehouse" dest-dir="{NNN_WAREHOUSE_DEST}" />
            <include artifact-path="SupportFiles" dest-dir="{SUPPORT_FILES_DEST}" />
        </build-asset>
    </inputs>
    <publish>
        <workingdir>sspl-base-build</workingdir>
        <destdir>sspl-base</destdir>
        <buildoutputs>
            <output name="output" srcfolder="output" artifactpath="output"> </output>
            <output name="openssl" srcfolder="redist/openssl" artifactpath="openssl"> </output>
            <output name="build_log" srcfolder="log"> </output>
        </buildoutputs>
        <publishbranches>master,feature,release,develop,bugfix</publishbranches>
    </publish>
</package>
"""

hosts_redirect = "127.0.0.1 ostia.eng.sophos\n"

def redirect_ostia_to_localhost():
    with open("/etc/hosts", "r+") as hosts_file:
        if hosts_redirect not in hosts_file.read():
            hosts_file.write(hosts_redirect)

def install_ostiant_cert_to_system(args):
    certInstallScript = os.path.join(args.workingdir, "utils", "InstallCertificateToSystem.sh")
    ostiant_public_cert = os.path.join(args.workingdir, "utils", "ostia-public-ca.crt")
    command = [certInstallScript, ostiant_public_cert]
    subprocess.run(command)

def fix_utils_scripts(args):
    subprocess.run(f"chmod +x {args.workingdir}/utils/*", shell=True)
    command = ["bash", "-c", "sed -i " + r"$'s/\r$//' " + f" {args.workingdir}/utils/*"]
    subprocess.run(command)


def fetch_artifacts(args):
    # below imports only needed here
    import certifi_local
    import tap.jwt
    from build_scripts.artisan_fetch import artisan_fetch

    tap.jwt.get_jwt(certifi_local.sophos.where_jwt_live_cert())

    release_package_path = os.path.join(args.workingdir, "release-package.xml")
    with open(release_package_path, 'w') as release_package_file:
        release_package_file.write(package_xml.format(args.workingdir, args.branch))
    artisan_fetch(release_package_path, build_mode="not_used", production_build=False, verbose=True)
    fix_utils_scripts(args)

def unzip_file_to_path_in_workingdir(path_to_zip, relative_path_to_dest, args):
    os.makedirs(os.path.join(args.workingdir, relative_path_to_dest), exist_ok=True)
    with zipfile.ZipFile(path_to_zip, 'r') as file:
        file.extractall(os.path.join(args.workingdir, relative_path_to_dest))

def setup_provided_artifacts(args):
    if args.use_vut:
        unzip_file_to_path_in_workingdir(args.vut_customer, VUT_CUSTOMER_DEST, args)
        unzip_file_to_path_in_workingdir(args.vut_warehouse, VUT_WAREHOUSE_DEST, args)

    if args.use_999:
        unzip_file_to_path_in_workingdir(args.nine_nine_nine_customer, NNN_CUSTOMER_DEST, args)
        unzip_file_to_path_in_workingdir(args.nine_nine_nine_warehouse, NNN_WAREHOUSE_DEST, args)

    unzip_file_to_path_in_workingdir(args.support_files, SUPPORT_FILES_DEST, args)
    fix_utils_scripts(args)

def setup_working_dir(args):
    if os.path.isdir(args.workingdir):
        shutil.rmtree(args.workingdir)
    os.mkdir(args.workingdir)

    if not args.offline_mode:
        fetch_artifacts(args)
    else:
        setup_provided_artifacts(args)

def inner_main(args):
    redirect_ostia_to_localhost()
    install_ostiant_cert_to_system(args)

    start_file_servers(args)

def start_file_servers(args):

    server_pem = os.path.join(args.workingdir, "utils", "server-private.pem")

    # logging_on = "--loggingOn"
    logging_on = ""
    cid_server_path = os.path.join(args.workingdir, "utils", "cidServer.py")
    cidlogfile = os.path.join(args.workingdir, "cidServer.log")
    cidlog = open(cidlogfile, "w")

    kill_previous_servers_command = "pgrep -f cidServer.py | xargs -i  kill {}"

    subprocess.run(kill_previous_servers_command, shell=True)

    command = ["python3", cid_server_path, "443", os.path.join(args.workingdir, "local_warehouses"), logging_on, f"--secure={server_pem}"]
    LOGGER.info(f"running {command}")
    subprocess.Popen(command, stdout=cidlog, stderr=cidlog)

    vut_dci_command = ["python3", cid_server_path, args.vut_port, os.path.join(args.workingdir, "local_warehouses/dev/sspl-warehouse/vut/warehouse/customer/"), logging_on, f"--secure={server_pem}"]
    subprocess.Popen(vut_dci_command, stdout=cidlog, stderr=cidlog)

    vut_dci_command = ["python3", cid_server_path, args.nine_nine_nine_port, os.path.join(args.workingdir, "local_warehouses/dev/sspl-warehouse/edr-mdr-999/warehouse/customer/"), logging_on, f"--secure={server_pem}"]
    subprocess.Popen(vut_dci_command, stdout=cidlog, stderr=cidlog)

if __name__ == "__main__":

    logging.root.setLevel(logging.INFO)
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--branch", default="develop", help="The port VUT warehouses will be hosted on")
    parser.add_argument('--workingdir', default=DEFAULT_WORKINGDIR, help='destination of created customer file')
    parser.add_argument("--vut-port", default="8000", help="The port VUT warehouses will be hosted on")
    parser.add_argument("--nine-nine-nine-port", default="8001", help="The port 999 warehouses will be hosted on")
    parser.add_argument('--no-gather', action='store_true')

    parser.add_argument('--offline-mode', default=False, action='store_true')
    parser.add_argument('--vut-customer', default=None)
    parser.add_argument('--vut-warehouse', default=None)
    parser.add_argument('--nine-nine-nine-customer', default=None)
    parser.add_argument('--nine-nine-nine-warehouse', default=None)
    parser.add_argument('--support-files', default=None)

    args = parser.parse_args()
    args.use_vut = False
    args.use_999 = False
    print(args)

    if args.offline_mode:
        valid = False
        if (args.vut_customer or args.vut_warehouse):
            if not (args.vut_customer and args.vut_warehouse and args.support_files):
                print("--vut-customer, --vut-warehouse, and --support-files must be provided together")
            else:
                valid = True
                args.use_vut = True

        if (args.nine_nine_nine_customer or args.nine_nine_nine_warehouse):
            if not (args.nine_nine_nine_customer and args.nine_nine_nine_warehouse and args.support_files):
                print("--nine-nine-nine-customer, --nine-nine-nine-warehouse, and --support-files must be provided together")
            else:
                valid = True
                args.use_999 = True

        if not valid:
            print("----------------------------------------------------------------------------------------")
            print("ERROR: Additional arguments must be provided in offline mode, see README.txt")
            print("----------------------------------------------------------------------------------------")
            exit(1)


    args.workingdir = os.path.join(args.workingdir, "ostiant")
    if not args.no_gather:
        setup_working_dir(args)

    inner_main(args)