#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.


from robot.api import logger

class OnFailureDump(object):
    def __init__(self):
        self.__m_logs = []

    def dump_failure_logs(self):
        for l in self.__m_logs:
            logger.info(l)


    def add_log_to_dump_on_failure(self, log):
        self.__m_logs.append(log)

