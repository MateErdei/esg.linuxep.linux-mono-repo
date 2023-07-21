"""
File-related utility methods.
"""
import os


def write_file(filename, data):
    """Write a buffer of binary data to a file"""
    with open(filename, 'wb') as outfile:
        outfile.write(data)


def read_file(filename):
    """Read the contents of a file into a binary buffer"""
    with open(filename, 'rb') as infile:
        data = infile.read()

    return data


def get_all_files(foldername):
    """
    Find all of the files under the given folder, recursively.
    The results will be relative to the given folder, and will
    not include subfolders, only files.
    """

    def get_files(foldername, current_root):
        found_files = []
        for f in os.scandir(foldername):
            if f.is_dir():
                found_files += get_files(f.path, os.path.join(current_root, f.name))
            else:
                found_files.append(os.path.join(current_root, f.name))

        return found_files

    return get_files(foldername, '.')
