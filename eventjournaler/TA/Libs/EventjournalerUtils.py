import os
import re
import stat

from robot.api import logger


def get_open_journal_file_from_directory(directory, basename=False):
    files = os.listdir(directory)
    return_value = None
    for file in files:
        if is_file_an_open_journal_file(file):
            if not return_value:
                return_value = os.path.join(directory, file)
            else:
                raise AssertionError(f"Found multiple open files in directory: {directory}")
    assert return_value is not None, "found no open files"
    if basename:
        return_value = os.path.basename(return_value)
    return return_value


def is_file_an_open_journal_file(file_name):
    if re.match(r"Detections-([0-9a-f]{16})-([0-9]{18})\.bin", file_name):
        return True


def get_number_of_open_files_in_directory(directory):
    n = 0
    files = os.listdir(directory)
    for file in files:
        if is_file_an_open_journal_file(file):
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


def generate_file(file_path, size_in_mb):
    size = int(size_in_mb)
    with open(file_path, 'wb') as fout:
        for i in range(1024*size):
            fout.write("a".encode("utf-8"))


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


def _check_correct_event_permissions(directory, files):
    for file in files:
        try:
            statbuf = os.stat(os.path.join(directory, file))
            mode = stat.S_IMODE(statbuf.st_mode)
            if mode != 0o640:
                raise AssertionError(f"Mode of {file} incorrect: should be 0o640 actually {mode:o}")
        except EnvironmentError:
            pass


def check_all_threat_events_are_correct_permissions(directory):
    files = os.listdir(directory)
    _check_correct_event_permissions(directory, files)


def check_one_threat_event_with_correct_permissions(directory):
    files = os.listdir(directory)
    if len(files) != 1:
        raise AssertionError(f"{directory} should contain one event")
    _check_correct_event_permissions(directory, files)
