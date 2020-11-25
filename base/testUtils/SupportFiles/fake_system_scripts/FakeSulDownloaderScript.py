#!/usr/bin/env python

#this script is used with FakeSulDownloader.py


import os
import sys
import shutil
import grp
from pwd import getpwnam
import time

import datetime
INSTALLPATH = os.getenv("SOPHOS_INSTALL", "/opt/sophos-spl")
CONFIGPATH = os.path.join(INSTALLPATH, "tmp" )
sys.path.append(CONFIGPATH)

def write_file( file_path, content):
  with open(file_path, 'w') as f:
    f.write( content )

logentry = open(os.path.join(CONFIGPATH,'fakesul.log'), 'a')

def log( content):
    tstamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
    logentry.write( tstamp + " : " + content + '\n')

import fakesul

sleeptime = 0
SIMULATION = 'NOTHING'
try:
  sleeptime = fakesul.SleepTime
except Exception as ex:
  print( "Failed to load SleepTime: " + str(ex))

try:
  SIMULATION = fakesul.SIMULATION
except Exception as ex:
  log( "Failed to load simulation: " + str(ex))

if ( sleeptime):
  import time
  log("Sleeping: " + str(sleeptime))
  time.sleep(sleeptime)

log( "Running simulation: " + str(SIMULATION))

if ( SIMULATION == "CREATEDONEFILE"):
  write_file( os.path.join(CONFIGPATH, 'done'), 'done')
elif ( SIMULATION == "NOTHING"):
  pass
elif ( SIMULATION == "COPYREPORT"):
  reportpath = os.path.join(CONFIGPATH, 'report.json')
  reportdest = os.path.join(INSTALLPATH, 'base/update/var/updatescheduler/update_report.json')
  log("Create file " + str(reportdest))
  uid = getpwnam('sophos-spl-updatescheduler').pw_uid
  gid = grp.getgrnam('root')[2]
  os.chown(reportpath, uid, gid)
  os.chmod(reportpath, 0o600)
  shutil.move(reportpath, reportdest)
  log("File content: {}".format(open(reportdest, 'r').read()))
else:
  log('Invalid command')
  raise AssertionError("Invalid command: " + str(SIMULATION))
log('Finished')