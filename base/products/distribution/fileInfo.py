#!/usr/bin/env python

from __future__ import absolute_import, print_function, division, unicode_literals

import hashlib
import operator
import os


def importFileList(p, dist):
    data = open(p).read()
    files = data.split(";")
    ret = []
    for f in files:
        f = f.strip()
        if f.startswith(dist):
            f = f[len(dist)+1:]
        ret.append(f)
    return ret

def walkDist(dist):
    ret = []
    for base, dirs, files in os.walk(dist):
        for f in files:
            f = os.path.join(base, f)
            if f.startswith(dist):
                f = f[len(dist)+1:]
            ret.append(f)
    return ret


class FileInfo(object):
    def __init__(self, dist, path):
        self.__m_fullPath = os.path.join(dist, path)
        self.m_path = path
        md5calc = hashlib.md5()
        sha1calc = hashlib.sha1()
        sha256calc = hashlib.sha256()
        sha384calc = hashlib.sha384()
        length = 0

        f = open(self.__m_fullPath)
        while True:
            data = f.read(1024*100)
            if len(data) == 0:
                break
            md5calc.update(data)
            sha1calc.update(data)
            sha256calc.update(data)
            sha384calc.update(data)
            length += len(data)

        if length < 1024:
            self.m_contents = data
        else:
            self.m_contents = None

        self.m_length = length
        self.m_md5 = md5calc.hexdigest()
        self.m_sha1 = sha1calc.hexdigest()
        self.m_sha256 = sha256calc.hexdigest()
        self.m_sha384 = sha384calc.hexdigest()

    def basename(self):
        return os.path.basename(self.m_path)

    def dirname(self):
        return os.path.dirname(self.m_path)

    def contents(self):
        if self.m_contents is None:
            raise Exception("Attempted to get contents for a large file: "+self.m_path)
        return self.m_contents

def load_file_info(dist, file_list_path):

    if file_list_path is not None and os.path.isfile(file_list_path):
        file_list = importFileList(file_list_path, dist)
    else:
        file_list = walkDist(dist)

    print(file_list)

    ## Hash all the files
    ret = [ FileInfo(dist, f) for f in file_list ]
    # noinspection PyTypeChecker
    ret.sort(key=operator.attrgetter('m_path'))
    return ret
