
import os

from robot.api import logger


class MockSystemExecutables(object):
    def __init__(self):
        self.__mocked_paths = {}

    def backup_system_executable(self, original_path, backup_suffix, destination_path=None):
        """
        If the original_path is not empty then back it up,
        otherwise mark destination_path to be deleted at end of test.
        :param original_path:
        :param backup_suffix:
        :param destination_path:
        :return:
        """
        if original_path == "":
            if destination_path is not None:
                self.__mocked_paths[destination_path] = None
            return

        assert(os.path.isfile(original_path))
        backup_path = original_path + backup_suffix
        os.replace(original_path, backup_path)
        self.__mocked_paths[original_path] = backup_path

    def remove_mock_system_executable_at_end_of_test(self, path):
        self.__mocked_paths[path] = None

    def restore_system_executables(self):
        for original, backup in self.__mocked_paths.items():
            if backup is None:
                logger.info(f"Removing {original}")
                os.remove(original)
            elif os.path.isfile(backup):
                logger.info(f"Restoring {backup} to {original}")
                os.replace(backup, original)
        self.__mocked_paths = {}
