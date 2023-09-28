# Copyright 2023 Sophos Limited. All rights reserved.
import sys


def main(argv):
    src_file_path = argv[1]
    string_to_replace = argv[2]
    file_path_to_insert = argv[3]
    final_file_path = argv[4]

    with open(src_file_path, "r") as f:
        original_file = f.read()

    with open(file_path_to_insert, "r") as f:
        insert_string = f.read()

    with open(final_file_path, "w") as f:
        f.write(original_file.replace(string_to_replace, insert_string))


if __name__ == "__main__":
    main(sys.argv)
