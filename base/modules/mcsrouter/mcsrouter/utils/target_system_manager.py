"""
target_system_manager Module
"""

import logging
LOGGER = logging.getLogger(__name__)

g_target_system = None
g_install_dir = None


def get_target_system(install_dir=None):
    """
    get_target_system
    """
    global g_target_system
    global g_install_dir
    if install_dir is None:
        install_dir = g_install_dir
    g_install_dir = install_dir
    if g_target_system is None:
        try:
            from mcsrouter import targetsystem
            g_target_system = targetsystem.TargetSystem(g_install_dir)
        except ImportError:
            try:
                import targetsystem
                g_target_system = targetsystem.TargetSystem(g_install_dir)
            except ImportError:
                # g_targetSystem = FakeTargetSystem()
                LOGGER.exception("Failed to import targetsystem")
    return g_target_system
