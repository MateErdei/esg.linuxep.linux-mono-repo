#!/usr/bin/env python

import fcntl
import os
import signal
import sys

import logging
logger = logging.getLogger(__name__)

childDeathPipe = None
sigtermPipe = None

def handler(signum, frame):
    global childDeathPipe,sigtermPipe
    if signum == signal.SIGCHLD:
        os.write(childDeathPipe[1],"1")
    elif signum in (signal.SIGTERM,signal.SIGINT):
        os.write(sigtermPipe[1],"1")
    else:
        print >>sys.stderr,"SIGNAL %d not handled"%signum

def makeNonBlockingAndNonInherit(fd):
    fl = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK | fcntl.FD_CLOEXEC)

def createPipe():
    pipe = os.pipe()
    makeNonBlockingAndNonInherit(pipe[0])
    makeNonBlockingAndNonInherit(pipe[1])
    return pipe

def closePipe(p):
    if p is not None:
        os.close(p[0])
        os.close(p[1])

def closePipes():
    global childDeathPipe,sigtermPipe

    closePipe(childDeathPipe)
    childDeathPipe = None

    closePipe(sigtermPipe)
    sigtermPipe = None

def setupSignalHandler():
    global childDeathPipe,sigtermPipe

    clearSignalHandler()

    childDeathPipe = createPipe()
    sigtermPipe = createPipe()

    signal.signal(signal.SIGCHLD,handler)
    signal.signal(signal.SIGINT,handler)
    signal.signal(signal.SIGTERM,handler)

def clearSignalHandler():
    ## Clear signal handlers so that signals don't get blocked
    signal.signal(signal.SIGCHLD,signal.SIG_IGN)
    signal.signal(signal.SIGINT,signal.SIG_IGN)
    signal.signal(signal.SIGTERM,signal.SIG_IGN)

    closePipes()
