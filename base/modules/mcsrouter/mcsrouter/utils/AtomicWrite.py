#!/usr/bin/env python

import os
import logging

def atomic_write(path, tmp_path, data):
    try:
        with open(tmp_path, "w") as f:
            f.write(data)
        os.rename(tmp_path, path)
    except e:
        logging.error("Atomic write failed with message: {}".format(e))

