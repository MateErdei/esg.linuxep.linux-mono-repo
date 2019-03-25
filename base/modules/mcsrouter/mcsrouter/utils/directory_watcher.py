#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Wrapper for the 'watcher' module, which writes to a pipe on any change,
so that we can wait for it in a select.select() call.
"""

from __future__ import print_function, division, unicode_literals

import os
import logging

import watchdog.observers
import watchdog.events

from . import signal_handler

LOGGER = logging.getLogger(__name__)


class PipeEventHandler(watchdog.events.PatternMatchingEventHandler):
    """
    Simple watchdog event handler to write to a pipe on any change
    """

    def __init__(
            self,
            pipe_file_descriptor,
            patterns=None,
            ignore_delete=False):
        """
        __init__
        """
        super(
            PipeEventHandler,
            self).__init__(
                ignore_directories=True,
                ignore_patterns=[
                    "*/.*",
                    "*~"],
                patterns=patterns)
        self.__pipe_file_descriptor = pipe_file_descriptor
        self.__ignore_delete = ignore_delete

    def on_any_event(self, event):
        """
        Write to the pipe whenever a change is observed
        """
        LOGGER.debug("Got event: %s", str(event))
        if self.__ignore_delete and event.event_type == "deleted":
            return
        os.write(self.__pipe_file_descriptor, b"1")


class DirectoryWatcher(object):
    """
    A simple directory watcher, which writes to a pipe whenever a change
    is observed
    """

    def __init__(self):
        """
        __init__
        """
        self.__pipe = signal_handler.create_pipe()
        self.__observer = watchdog.observers.Observer()
        self.__observer.start()

    def __del__(self):
        """
        __del_
        """
        self.__observer.stop()
        self.__observer.join()
        signal_handler.close_pipe(self.__pipe)

    @property
    def notify_pipe_file_descriptor(self):
        """
        get the readfd of the notification pipe (for use in a select() call)
        """
        return self.__pipe[0]

    def add_watch(self, directory, patterns=None, ignore_delete=False):
        """
        add a directory to watch
        """
        LOGGER.debug("Adding directory watch for: %s", directory)
        event_handler = PipeEventHandler(
            self.__pipe[1],
            patterns=patterns,
            ignore_delete=ignore_delete)
        self.__observer.schedule(event_handler, directory)
