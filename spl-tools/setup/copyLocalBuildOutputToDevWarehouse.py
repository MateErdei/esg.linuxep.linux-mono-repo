#!/usr/bin/python

import os
import shutil
import socket

HOSTNAME = socket.gethostname()
assert HOSTNAME.lower() != "master", "your machine cannot be called master"

BFR_PATH = os.path.join("/mnt", "filer6", "bfr")
def require_filer6_bfr_exists():
    assert os.path.isdir(BFR_PATH), "Could not find filer6 bfr at: {}".format(BFR_PATH)
require_filer6_bfr_exists()

ALLOWED_PRODUCT_BFR_PATHS = [
    "/mnt/filer6/bfr/sspl-base",
    "/mnt/filer6/bfr/sspl-mdr-componentsuite",
    "/mnt/filer6/bfr/sspl-edr-plugin"
]


SSPL_TOOLS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
assert os.path.basename(SSPL_TOOLS) == "spl-tools" and os.path.isdir(SSPL_TOOLS)

# It doesn't matter what this is, as long as it works. This one works.
DEFAULT_BUILD_TIMESTAMP= "20191014161549-a"
# This version isn't used for anything, it is a directory name between the build timestamp and output folder
DEFAULT_VERSION_DIRECTORY_NAME = "1.0.0"
OUTPUT_DIRECTORY_NAME = "output"

class product():
    def __init__(self, bfr_name, local_name):
        self.bfr_name = bfr_name
        self.local_name = local_name
        self.product_bfr_path = os.path.join(BFR_PATH, self.bfr_name)
        assert self.product_bfr_path in ALLOWED_PRODUCT_BFR_PATHS
        self.host_branch_path = os.path.join(self.product_bfr_path, HOSTNAME)
        self.last_good_build_number = DEFAULT_BUILD_TIMESTAMP

        self.last_good_build_path = os.path.join(self.host_branch_path, "{}_lastgoodbuild.txt".format(self.bfr_name))

    def require_product_exists_on_filer6(self):
        assert os.path.isdir(self.product_bfr_path), "{} not found on bfr".format(self.bfr_name)

    def host_branch_exists_for_product(self):
        return os.path.isdir(self.host_branch_path)

    def require_host_branch_exists_for_product(self):
        if not self.host_branch_exists_for_product():
            raise AssertionError("{} branch not found for {} on bfr".format(HOSTNAME, self.bfr_name))

    def require_host_branch_does_not_exist_for_product(self):
        if self.host_branch_exists_for_product():
            raise AssertionError("host branch already exists for {} at {}".format(self.bfr_name, self.host_branch_path))

    def creat_last_good_build_file(self):
        with open(self.last_good_build_path, "w") as last_good_build_file:
            last_good_build_file.write(DEFAULT_BUILD_TIMESTAMP)

    def get_version_directory_name(self, parent_directory):
        if not os.path.isdir(parent_directory):
            raise AssertionError("Parent directory \"{}\" does not exist".format(parent_directory))
        parent_directory_contents = os.listdir(parent_directory)
        assert len(parent_directory_contents) == 1, "parent directory contains more than the expected 1 version folder"
        version_folder_name = parent_directory_contents[0]
        return version_folder_name

    def get_default_bfr_output_directory_path(self):
        bfr_output_directory_path = os.path.join(self.host_branch_path, DEFAULT_BUILD_TIMESTAMP, self.bfr_name, DEFAULT_VERSION_DIRECTORY_NAME, OUTPUT_DIRECTORY_NAME)
        return bfr_output_directory_path

    def get_bfr_output_directory_path(self):
        bfr_output_directory_path = self.get_default_bfr_output_directory_path()

        if os.path.isdir(bfr_output_directory_path):
            return bfr_output_directory_path
        raise AssertionError("{} does not exist".format(bfr_output_directory_path))

    def get_local_repo_output_path(self):
        local_repo_path = os.path.join(SSPL_TOOLS, self.local_name)
        if os.path.isdir(local_repo_path):
            local_output_directory = os.path.join(local_repo_path, OUTPUT_DIRECTORY_NAME)
            if os.path.isdir(local_output_directory):
                return local_output_directory
            else:
                raise AssertionError("output folder not found for {} at {}".format(self.local_name, local_output_directory))
        raise AssertionError("{} not found on local machine at {}".format(self.local_name, local_repo_path))


    def make_required_bfr_directories(self):
        bfr_output_directory_path = self.get_default_bfr_output_directory_path()
        os.makedirs(bfr_output_directory_path)


    def setup_host_branch_directory(self):
        try:
            require_filer6_bfr_exists()
            self.require_product_exists_on_filer6()
            self.require_host_branch_does_not_exist_for_product()

            self.make_required_bfr_directories()
            self.creat_last_good_build_file()
        except AssertionError as exception:
            print("ERROR: failed to setup {} branch on filer6fr for {} with error: {}".format(self.bfr_name, HOSTNAME, exception))
            return False
        return True

    def copy_from_local_to_bfr(self):
        try:
            self.require_product_exists_on_filer6()
            self.require_host_branch_exists_for_product()

            local_output_directory_path = self.get_local_repo_output_path()
            bfr_output_directory = self.get_bfr_output_directory_path()

            print("copying {} to {}".format(local_output_directory_path, bfr_output_directory))
            shutil.rmtree(bfr_output_directory)
            shutil.copytree(local_output_directory_path, bfr_output_directory)
        except AssertionError as exception:
            print("ERROR: Failed to copy with error: {}".format(exception))
            return False
        return True

BASE = product("sspl-base", "everest-base")
MTR = product("sspl-mdr-componentsuite", "sspl-plugin-mdr-componentsuite")
EDR = product("sspl-edr-plugin", "sspl-plugin-edr-component")

def main():
    base_success = BASE.copy_from_local_to_bfr()
    mtr_success = MTR.copy_from_local_to_bfr()
    edr_success = EDR.copy_from_local_to_bfr()

    if base_success and mtr_success and edr_success:
        print("""SUCCESS
You now need to kick off the ci build for your pairing stations sspl-warehouse branch

If it doesn't yet have one, make a branch from master named "develop/${pairingStationName}" (all lower case)
and replace all uncommented references to master in "def/common/components.yaml"

Once the ci build is completed successfully, change the default value for 
"OSTIA_VUT_ADDRESS_BRANCH" in "${systemproducttests}/libs/WarehouseUtils.py"
to develop-${pairingStationName}. This will set your pairing station to use your warehouse.
""")

if __name__ == '__main__':
    main()
