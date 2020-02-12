import sys
import os
from robot.api.deco import keyword

@keyword
def create_long_path(dirname, depth, file=".", file_contents=""):
    os.chdir(root)

    try:
        for i in range(depth):
            os.mkdir(dirname)
            os.chdir(dirname)
    except Exception as e:
        print("Failed to create deep tree")
        print(e)
        sys.exit(2)

    with open(file, 'w') as file:
        file.write(file_contents)
        file_path = file.name

    return file_path
