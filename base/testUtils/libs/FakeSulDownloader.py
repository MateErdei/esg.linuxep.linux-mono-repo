#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import json
import shutil
from robot.api import logger

INSTALLPATH = os.getenv("SOPHOS_INSTALL", "/opt/sophos-spl")
BINPATH = os.path.join(INSTALLPATH, "base/bin" )
CONFIGPATH = os.path.join(INSTALLPATH, "tmp" )

FAKESULNAME = 'FakeSulDownloader.py'
FAKESULPATH = os.path.join(BINPATH,FAKESULNAME )
FAKESULCONFIG = os.path.join(CONFIGPATH,'fakesul.py' )


#SIMULATION: NOTHING, COPYREPORT, CREATEDONEFILE

FakeSulDownloaderScriptPath = os.path.join('.', 'SupportFiles/fake_system_scripts/FakeSulDownloaderScript.py')


def write_file(file_path, content):
    with open(file_path, 'w') as f:
        f.write( content)


def write_fake_downloader():
    shutil.copy(FakeSulDownloaderScriptPath, FAKESULPATH)
    os.chmod(FAKESULPATH, 0o700)


TIMES = {
    1: "20180821 121220",
    2: "20180822 121220",
    3: "20180823 121220",
    4: "20180824 121220"
}

def productDict(rigidName ="ServerProtectionLinux-Base",
               productName="ServerProtectionLinux-Base#0.5.0",
               downloadVersion="0.5.0",
               errorDescription="",
               productStatus = "UPDTODATE", **kwargs):
    if productStatus not in ['UPTODATE', 'INSTALLFAILED', 'UNINSTALLFAILED', 'VERIFYFAILED', 'UPGRADED', 'SYNCFAILED', 'UNINSTALLED']:
        raise AssertionError("Invalid product status selected: " + str(productStatus))
    return { "rigidName": rigidName,
             "productName": productName,
             "downloadVersion": downloadVersion,
             "errorDescription": errorDescription,
             "productStatus": productStatus }

def downloadReportDict(startTime=1,
                       syncTime=1,
                       status="SUCCESS",
                       sulError="",
                       errorDescription="",
                       urlSource="Sophos", **kwargs
                       ):
    if status not in ['SUCCESS', 'INSTALLFAILED', 'DOWNLOADFAILED', 'RESTARTNEEDED', 'CONNECTIONERROR','PACKAGESOURCEMISSING', 'UNINSTALLFAILED', 'UNSPECIFIED' ]:
        raise AssertionError("Invalid report status selected: " + str(status))
    stime = TIMES[int(startTime)]
    sync = TIMES[int(syncTime)]
    report = { "startTime": stime,
               "finishTime": stime,
               "syncTime": sync,
               "status": status,
               "sulError": sulError,
               "errorDescription": errorDescription,
               "urlSource": urlSource,
               "products": [ ]
               }
    return report

def addProduct( reportDict, productDict):
    reportDict['products'].append(productDict)
    return reportDict

def create_json_download_report(reportDict):
    return json.dumps(reportDict, separators=(',', ': '), indent=4)


class FakeSulDownloader(object):
    def __init__(self):
        self.sul_downloader_link = False
        pass

    def setup_base_and_plugin_upgraded(self, startTime=1, syncTime=1):
        self.__base_and_plugin('UPGRADED', startTime=startTime, syncTime=syncTime)

    def setup_base_and_plugin_sync_and_uptodate(self, startTime=1, syncTime=1):
        self.__base_and_plugin('UPTODATE', startTime=startTime, syncTime=syncTime )

    def setup_base_uptodate_via_update_cache(self, updateCacheUrl):
        self.__base_and_plugin('UPTODATE', startTime=1, syncTime=1, urlSource=updateCacheUrl )

    def setup_plugin_install_failed(self, startTime=2, syncTime=1):
        report = self.__base_and_plugin_report(productStatus='UPTODATE', startTime=startTime, syncTime=syncTime)
        report['products'][1]['productStatus'] = 'INSTALLFAILED'
        report['products'][1]['errorDescription'] = 'Failed to install'
        report['status'] = 'INSTALLFAILED'
        self.__setup_simulate_report(report)


    def __base_and_plugin(self, productStatus, **kwargs):
        report = self.__base_and_plugin_report(productStatus=productStatus, **kwargs)
        self.__setup_simulate_report(report)

    def __base_and_plugin_report(self, **kwargs):
        report = downloadReportDict(**kwargs)
        base = productDict(**kwargs)
        plugin = productDict(rigidName="ServerProtectionLinux-Plugin",productName="ServerProtectionLinux-Plugin#0.5", **kwargs)
        addProduct(report, base)
        addProduct(report, plugin)
        return report

    def __setup_simulate_report(self, report):
        logger.debug( "Simulate SulDownloader will produce the report: {}".format(repr(report)))
        json_content = create_json_download_report(report)
        self.setup_copy_file(json_content, 1)

    def __create_fakesul(self, content):
        self.create_link()
        write_file(FAKESULCONFIG, content)
        pyc_file = FAKESULCONFIG + 'c'
        if os.path.exists(pyc_file):
            try:
                os.remove(FAKESULCONFIG+'c')
            except Exception as ex:
                logger.warn("Exception on removing the pyc file which could make python to load previous fakesul.py file. Error: {}".format(repr(ex)))

    def create_link(self):
        if self.sul_downloader_link:
            return
        write_fake_downloader()
        currdir = os.getcwd()
        os.chdir(BINPATH)
        try:
            os.unlink("SulDownloader")
            os.symlink(FAKESULNAME, "SulDownloader")
        except Exception as ex:
            raise AssertionError("Link creation failed: " + str(ex))
        os.chdir(currdir)
        self.sul_downloader_link = True

    def setup_done_file(self):
        try:
            os.unlink(os.path.join(CONFIGPATH, 'done'))
        except EnvironmentError as ex:
            # Don't warn if the file doesn't exist - ENOENT == 2
            if ex.errno != 2:
                logger.warn( "Exception on unlinking suldownloader config file {}".format(repr(ex)))
        self.__create_fakesul("""SIMULATION="CREATEDONEFILE"
""")

    def setup_copy_file(self, content, sleeptime):
        write_file(os.path.join(CONFIGPATH, 'report.json'), content)
        self.__create_fakesul("""SIMULATION="COPYREPORT"
SleepTime=%s""" %(sleeptime))

    def restore_link(self):
        if not self.sul_downloader_link:
            return
        currdir = os.getcwd()
        os.chdir(BINPATH)
        try:
          os.unlink("SulDownloader")
          os.symlink("SulDownloader.0", "SulDownloader")
        except Exception as ex:
            raise AssertionError("Restore link failed: " + str(ex))

        os.chdir(currdir)
        self.sul_downloader_link = False

    def __del__(self):
        if self.sul_downloader_link:
            self.restore_link()
            self.sul_downloader_link = False


