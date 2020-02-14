import sys
import os


def create_long_path(dirname, depth, root='/', file="file", file_contents=""):
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

    return os.getcwd()
