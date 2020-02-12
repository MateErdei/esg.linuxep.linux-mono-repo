import sys
import os
from robot.api import logger


def create_long_path(dirname, depth, root='/', file="file", file_contents=""):
    os.chdir(root)

    try:
        logger.console('Creating deep folder tree')
        for i in range(depth):
            if not os.path.exists(dirname):
                os.mkdir(dirname)
            os.chdir(dirname)
    except Exception as e:
        logger.console("Failed to create deep tree")
        logger.console(e)
        sys.exit(2)

    logger.console('Creating file')
    with open(file, 'w') as file:
        file.write(file_contents)
        file_path = file.name

    return os.getcwd()
