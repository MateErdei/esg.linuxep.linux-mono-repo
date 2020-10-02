import logging.handlers
import os
from .PathsLocation import get_install_location

def setup_logging(filename, name):
    logger = logging.getLogger(name)
    #Remove any previous handlers which may exist on global logger
    handlers = logger.handlers

    for handler in handlers:
        logger.removeHandler(handler)

    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(asctime)s %(levelname)s %(name)s: %(message)s")

    log_dir = get_log_dir()
    if not os.path.isdir(log_dir):
        os.makedirs(log_dir)
    logfile = os.path.join(log_dir, filename)

    file_handler = logging.handlers.RotatingFileHandler(logfile, maxBytes=1024 * 1024, backupCount=5)
    file_handler.setFormatter(formatter)
    file_handler.setLevel(logging.DEBUG)
    logger.addHandler(file_handler)
    return logger


def get_logger(name):
    return logging.getLogger(name)


def get_log_dir():
    return os.path.join(get_install_location())
