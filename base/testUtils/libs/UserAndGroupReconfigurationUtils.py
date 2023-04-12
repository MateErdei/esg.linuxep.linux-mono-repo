#!/usr/bin/env python
# Copyright 2023-2023 Sophos Limited. All rights reserved.

import glob
import logging
import os
import pathlib
import sys

try:
    from testUtils.libs.OSUtils import get_file_owner_id, get_file_group_id
except ModuleNotFoundError as exception:
    from OSUtils import get_file_owner_id, get_file_group_id


def get_user_and_group_ids_of_files(directory: str):
    ids_of_dir_entries = {}
    for file in pathlib.Path(directory).glob("**/*"):
        file = str(file)
        if os.path.exists(file):
            user_id = get_file_owner_id(file)
            group_id = get_file_group_id(file)
            ids_of_dir_entries[file] = {
                "user_id": user_id,
                "group_id": group_id
            }
    return ids_of_dir_entries


def check_changes_of_user_ids(ids_of_dir_entries_before, ids_of_dir_entries_after, prev_user_id, new_user_id):
    for file, ids in ids_of_dir_entries_before.items():
        if file not in ids_of_dir_entries_after:
            logging.info(f"The file {file} was not found in the 'after' set of files and directories")
            continue
        if ids["user_id"] == prev_user_id:
            if ids_of_dir_entries_after[file]["user_id"] != new_user_id:
                raise AssertionError(f"User ID of {file} was not changed from {prev_user_id} to {new_user_id}, actual ID {ids_of_dir_entries_after[file]['user_id']}")
            else:
                print(f"Changed OK: {file} owner ID changed from {prev_user_id} to {new_user_id}")


def check_changes_of_group_ids(ids_of_dir_entries_before, ids_of_dir_entries_after, prev_group_id, new_group_id):
    for file, ids in ids_of_dir_entries_before.items():
        if file not in ids_of_dir_entries_after:
            logging.info(f"The file {file} was not found in the 'after' set of files and directories")
            continue
        if ids["group_id"] == prev_group_id:
            if ids_of_dir_entries_after[file]["user_id"] != new_group_id:
                raise AssertionError(f"Group ID of {file} was not changed from {prev_group_id} to {new_group_id}, actual ID {ids_of_dir_entries_after[file]['user_id']}")
            else:
                print(f"Changed OK: {file} group ID changed from {prev_group_id} to {new_group_id}")


# For manual verification and testing of these utils, needs to be run as root.
def main():
    test_dir = os.path.join("/tmp", os.path.basename(__file__).replace(".py", ""))
    print(f"test_dir = {test_dir}")
    test_sub_dir = os.path.join(test_dir, "subdir")
    print(f"test_sub_dir = {test_sub_dir}")
    os.makedirs(test_sub_dir, exist_ok=True)

    file_1 = os.path.join(test_dir, "testfile1.txt")
    file_2 = os.path.join(test_sub_dir, "testfile2.txt")

    with open(file_1, "w") as file_writer:
        file_writer.write("test")

    with open(file_2, "w") as file_writer:
        file_writer.write("test")

    id_before = 2123
    id_after = 2124

    # Initial setup
    os.chown(test_sub_dir, id_before, id_before)
    os.chown(file_1, id_before, id_before)
    os.chown(file_2, id_before, id_before)
    # IDs before
    ids_before = get_user_and_group_ids_of_files(test_dir)
    print("IDs Before")
    print(ids_before)

    # After
    os.chown(test_sub_dir, id_after, id_after)
    os.chown(file_1, id_after, id_after)
    os.chown(file_2, id_after, id_after)
    # IDs after
    ids_after = get_user_and_group_ids_of_files(test_dir)
    print("IDs After")
    print(ids_after)

    # Validate
    check_changes_of_user_ids(ids_before, ids_after, str(id_before), str(id_after))
    check_changes_of_group_ids(ids_before, ids_after, str(id_before), str(id_after))
    return 0


if __name__ == "__main__":
    sys.exit(main())


