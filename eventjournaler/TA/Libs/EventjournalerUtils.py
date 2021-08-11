import os
import re

def get_open_journal_file_from_directory(directory, basename=False):
    files = os.listdir(directory)
    return_value = None
    for file in files:
        if file.endswith(".bin"):
            if len(file.split("-")) == 3:
                if not return_value:
                    return_value = os.path.join(directory, file)
                else:
                    raise AssertionError(f"Found multiple open files in directory: {directory}")
    assert return_value, "found no open files"
    if basename:
        return_value = os.path.basename(return_value)
    return return_value


def get_number_of_open_files_in_directory(directory):
    n = 0
    files = os.listdir(directory)
    for file in files:
        if re.match(r"Detections-([0-9a-f]{16})-([0-9]{18})\.bin", file):
            n += 1
    return n

def get_number_of_closed_files_in_directory(directory):
    n = 0
    files = os.listdir(directory)
    for file in files:
        if re.match(r"Detections-([0-9a-f]{16})-([0-9a-f]{16})-([0-9]{18})-([0-9]{18})\.bin", file):
            n += 1
    return n

def get_number_of_compressed_files_in_directory(directory):
    n = 0
    files = os.listdir(directory)
    for file in files:
        if re.match(r"Detections-([0-9a-f]{16})-([0-9a-f]{16})-([0-9]{18})-([0-9]{18})\.xz", file):
            n += 1
    return n


def assert_expected_file_in_directory(directory, expected_closed_files=0, expected_compressed_files=0, expected_open_files=0):
    actual_closed_files = get_number_of_closed_files_in_directory(directory)
    assert actual_closed_files == expected_closed_files, f"expected {expected_closed_files} closed files, got {actual_closed_files}"
    actual_compressed_files = get_number_of_compressed_files_in_directory(directory)
    assert actual_compressed_files == expected_compressed_files, f"expected {expected_compressed_files} compressed files, got {actual_compressed_files}"
    actual_open_files = get_number_of_open_files_in_directory(directory)
    assert actual_open_files == expected_open_files, f"expected {expected_open_files} closed files, got {actual_open_files}"

    number_of_files_in_directory = len(os.listdir(directory))
    sum_of_expected_files = (expected_closed_files + expected_compressed_files + expected_open_files)

    assert number_of_files_in_directory == sum_of_expected_files, f"number of journal files {number_of_files_in_directory} did not equal sum of expected files: {sum_of_expected_files}"