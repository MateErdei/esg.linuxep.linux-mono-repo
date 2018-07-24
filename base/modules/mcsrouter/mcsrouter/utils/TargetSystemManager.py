

import logging
logger = logging.getLogger(__name__)

g_targetSystem = None
g_installDir = None

def getTargetSystem(installDir=None):
    global g_targetSystem
    global g_installDir
    if installDir is None:
        installDir = g_installDir
    g_installDir = installDir
    if g_targetSystem is None:
        try:
            from mcsrouter import targetsystem
            g_targetSystem = targetsystem.TargetSystem(g_installDir)
        except ImportError:
            try:
                import targetsystem
                g_targetSystem = targetsystem.TargetSystem(g_installDir)
            except ImportError:
                # g_targetSystem = FakeTargetSystem()
                logger.exception("Failed to import targetsystem")
    return g_targetSystem
