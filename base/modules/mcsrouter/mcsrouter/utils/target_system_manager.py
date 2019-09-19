# Copyright 2019 Sophos Plc, Oxford, England.
"""
target_system_manager Module
"""

import logging
LOGGER = logging.getLogger(__name__)

G_TARGET_SYSTEM = None
G_INSTALL_DIR = None


def get_target_system(install_dir=None):
    """
    get_target_system
    """
    #pylint: disable=global-statement
    global G_TARGET_SYSTEM
    global G_INSTALL_DIR
    if install_dir is None:
        install_dir = G_INSTALL_DIR
    G_INSTALL_DIR = install_dir
    if G_TARGET_SYSTEM is None:
        try:
            from mcsrouter import targetsystem
            G_TARGET_SYSTEM = targetsystem.TargetSystem(G_INSTALL_DIR)
        except ImportError:
            try:
                import targetsystem
                G_TARGET_SYSTEM = targetsystem.TargetSystem(G_INSTALL_DIR)
            except ImportError:
                # g_targetSystem = FakeTargetSystem()
                LOGGER.exception("Failed to import targetsystem")
    return G_TARGET_SYSTEM
