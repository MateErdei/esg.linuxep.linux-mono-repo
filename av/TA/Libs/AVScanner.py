import sys
import os


def create_long_path(dirname, depth, root='/', file="file", file_contents=""):
    directory_to_return_process_to = os.getcwd()
    os.chdir(root)

    try:
        for i in range(depth):
            if not os.path.exists(dirname):
                os.mkdir(dirname)
            os.chdir(dirname)
    except Exception as e:
        sys.exit(2)

    with open(file, 'w') as file:
        file.write(file_contents)
        file_path = file.name

    directory_to_return = os.getcwd()
    os.chdir(directory_to_return_process_to)

    return directory_to_return
