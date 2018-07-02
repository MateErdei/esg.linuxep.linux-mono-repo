##Python library for doing subprocesses

from __future__ import print_function,division,unicode_literals

import subprocess
import signal


def subprocess_setup():
    # Python installs a SIGPIPE handler by default. This is usually not what
    # non-Python subprocesses expect.
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)


def run(command):
    retCode = subprocess.call(command, preexec_fn=subprocess_setup)
    if retCode == None:
        retCode = 0
    return retCode


def runSilentOutput(command):
    null = open("/dev/null", "w")
    retCode = subprocess.call(command, stdout=null,preexec_fn=subprocess_setup)
    null.close()
    if retCode is None:
        retCode = 0
    return retCode


def collectOutput(command, stdin):
    proc = subprocess.Popen(command, stdout=subprocess.PIPE,stdin=subprocess.PIPE,preexec_fn=subprocess_setup)
    output = proc.communicate(stdin)[0]
    retCode = proc.wait()
    if retCode is None:
        retCode = 0
    return (retCode,output)
