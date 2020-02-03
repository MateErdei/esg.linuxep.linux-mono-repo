import os
from watchdog.observers import Observer
from watchdog.events import PatternMatchingEventHandler


class FilesystemWatcher(object):
    def __init__(self):
        self.ignore_patterns = []

    def setup_filesystem_watcher(self, path, log_file_path="FilesystemWatcher.log", match_patterns="*", ignore_patterns=None, ignore_directories=True):
        self.log_file_path = log_file_path
        self.clear_filesystem_watcher_log()
        self.patterns = match_patterns
        if ignore_patterns is not None:
            self.ignore_patterns = ignore_patterns

        self.ignore_patterns.append("*{}".format(self.log_file_path))
        self.ignore_directories = ignore_directories
        self.case_sensitive = True
        self.event_handler = PatternMatchingEventHandler(self.patterns, self.ignore_patterns, self.ignore_directories, self.case_sensitive)

        self.event_handler.on_created = self.on_created
        self.event_handler.on_deleted = self.on_deleted
        self.event_handler.on_modified = self.on_modified
        self.event_handler.on_moved = self.on_moved

        self.path = path
        self.go_recursively = True
        self.observer = Observer()
        self.observer.schedule(self.event_handler, self.path, recursive=self.go_recursively)

    def _log(self, msg):
        with open(self.log_file_path, 'a+') as log_file:
            log_file.write("{}\n".format(msg))

    def on_created(self, event):
        self._log("created: {}".format(event.src_path))

    def on_deleted(self, event):
        self._log("deleted: {}".format(event.src_path))

    def on_modified(self, event):
        self._log("modified: {}".format(event.src_path))

    def on_moved(self, event):
        self._log("moved: {} to: {}".format(event.src_path, event.dest_path))

    def start_filesystem_watcher(self):
        self.observer.start()

    def stop_filesystem_watcher(self):
        self.observer.stop()
        self.observer.join()

    def clear_filesystem_watcher_log(self):
        if os.path.exists(self.log_file_path):
            os.remove(self.log_file_path)
