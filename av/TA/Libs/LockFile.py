#!/usr/bin/env python3

import fcntl


class LockFile(object):
    ROBOT_LIBRARY_SCOPE = 'GLOBAL'

    def __init__(self):
        self.__file_handle = None

    def open_and_acquire_lock(self, lock_file):
        self.__file_handle = open(lock_file, 'w+')
        fcntl.flock(self.__file_handle, fcntl.LOCK_EX)

    def release_lock(self):
        self.__file_handle.close()
