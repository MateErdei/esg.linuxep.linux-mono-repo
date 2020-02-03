import logging.handlers
import os


def setup_logging(filename, name):
    logger = logging.getLogger(name)
    #Remove any previous handlers which may exist on global logger
    handlers = logger.handlers
    for handler in handlers:
        logger.removeHandler(handler)
    logging.shutdown(handlers)
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

    stream_handler = logging.StreamHandler()
    stream_handler.setFormatter(formatter)
    stream_handler.setLevel(logging.DEBUG)
    logger.addHandler(stream_handler)
    return logger

def get_log_dir():
    return os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../tmp/"))
