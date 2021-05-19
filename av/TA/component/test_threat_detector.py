import inspect
import sys
import subprocess

import logging
logger = logging.getLogger("test_threat_detector")
logger.setLevel(logging.DEBUG)


def test_start_threat_detector(sophos_threat_detector):
    logger.debug("Starting %s", inspect.currentframe().f_code.co_name)
    # Start sophos_threat_detector
    sophos_threat_detector.start()

    # Stop sophos_threat_detector
    sophos_threat_detector.stop()
    logger.debug("Completed %s", inspect.currentframe().f_code.co_name)


def test_threat_detector_signal_logging(sspl_mock, av_plugin_instance):
    logger.debug("Starting %s", inspect.currentframe().f_code.co_name)

    # Start sophos_threat_detector

    # send signal
    logger.debug("Completed %s", inspect.currentframe().f_code.co_name)