import unittest

import PathManager

import fcntl
import os
import mcsrouter.mcs_router

PIDFILE="tmp/var/lock-sophosspl/mcsrouter.pid"

class TestPidFile(unittest.TestCase):
    def setup(self):
        PathManager.safeDelete(PIDFILE)

    def testCreation(self):
        installDir = os.path.abspath("./tmp")
        pidfile = mcsrouter.mcs_router.PidFile(installDir)
        self.assertTrue(os.path.isfile(PIDFILE))
        with open(PIDFILE) as pid_handle:
            pid = pid_handle.read().strip()
        self.assertEqual(pid, str(os.getpid()))
        pidfile.exit()
        self.assertFalse(os.path.isfile(PIDFILE))

    def testRepeat(self):
        installDir = os.path.abspath("./tmp")
        pidfile = mcsrouter.mcs_router.PidFile(installDir)
        self.assertTrue(os.path.isfile(PIDFILE))
        with open(PIDFILE) as pid_handle:
            pid = pid_handle.read().strip()
        self.assertEqual(pid, str(os.getpid()))
        pidfile.exit()
        self.assertFalse(os.path.isfile(PIDFILE))

        pidfile = mcsrouter.mcs_router.PidFile(installDir)
        self.assertTrue(os.path.isfile(PIDFILE))
        with open(PIDFILE) as pid_handle:
            pid = pid_handle.read().strip()
        self.assertEqual(pid, str(os.getpid()))
        pidfile.exit()
        self.assertFalse(os.path.isfile(PIDFILE))

if __name__ == '__main__':
    unittest.main()


