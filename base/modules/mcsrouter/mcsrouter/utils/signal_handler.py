#!/usr/bin/env python3
"""
signal_handler Module
"""

import fcntl
import logging
import os
import signal
import sys

LOGGER = logging.getLogger(__name__)

#pylint: disable=invalid-name, global-statement

subprocess_exit_pipe = None
sig_term_pipe = None


def handler(sig_num, frame):  #pylint: disable=unused-argument
    """
    handler
    """
    global subprocess_exit_pipe, sig_term_pipe
    if sig_num == signal.SIGCHLD:
        os.write(subprocess_exit_pipe[1], b"1")
    elif sig_num in (signal.SIGTERM, signal.SIGINT):
        os.write(sig_term_pipe[1], b"1")
    else:
        print("SIGNAL {} not handled".format(sig_num), file=sys.stderr)


def make_non_blocking_and_non_inherit(file_descriptor):
    """
    make_non_blocking_and_non_inherit
    """
    flags = fcntl.fcntl(file_descriptor, fcntl.F_GETFL)
    fcntl.fcntl(file_descriptor, fcntl.F_SETFL, flags |
                os.O_NONBLOCK | fcntl.FD_CLOEXEC)


def create_pipe():
    """
    create_pipe
    """
    pipe = os.pipe()
    make_non_blocking_and_non_inherit(pipe[0])
    make_non_blocking_and_non_inherit(pipe[1])
    return pipe


def close_pipe(pipe):
    """
    close_pipe
    """
    if pipe is not None:
        os.close(pipe[0])
        os.close(pipe[1])


def close_pipes():
    """
    close_pipes
    """
    global subprocess_exit_pipe, sig_term_pipe

    close_pipe(subprocess_exit_pipe)
    subprocess_exit_pipe = None

    close_pipe(sig_term_pipe)
    sig_term_pipe = None


def setup_signal_handler():
    """
    setup_signal_handler
    """
    global subprocess_exit_pipe, sig_term_pipe

    clear_signal_handler()

    subprocess_exit_pipe = create_pipe()
    sig_term_pipe = create_pipe()

    signal.signal(signal.SIGCHLD, handler)
    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)


def clear_signal_handler():
    """
    clear_signal_handler
    """
    # Clear signal handlers so that signals don't get blocked
    signal.signal(signal.SIGCHLD, signal.SIG_IGN)
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    signal.signal(signal.SIGTERM, signal.SIG_IGN)

    close_pipes()
