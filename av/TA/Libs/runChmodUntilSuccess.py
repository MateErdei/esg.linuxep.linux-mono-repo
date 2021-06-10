import os
import sys


def run_chmod_until_success(path):
    while True:
        try:
            os.chmod(path, 000)
        except EnvironmentError:
            pass


if __name__ == "__main__":
    run_chmod_until_success(sys.argv[0])
