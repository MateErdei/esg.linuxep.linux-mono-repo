# Copyright 2022 Sophos Limited. All rights reserved.
import os


class CodeFile:
    def __init__(self, git_repo, filename):
        self.__git_repo = git_repo
        self.__filename = filename
        self.__changed = False
        self.__external_update = False
        self.__lines = None
        self.load()

    def git_repo(self):
        return self.__git_repo

    def git_path(self):
        return os.path.normpath(self.__filename[len(self.__git_repo.working_dir) + 1:])

    def filename(self):
        return self.__filename

    def lines(self):
        return self.__lines

    def update(self, lines):
        self.__lines = lines
        self.__changed = True

    def external_update(self):
        self.__external_update = True

    def updated(self):
        return self.__changed or self.__external_update

    def save(self):
        if self.__changed:
            open(self.__filename, 'w', encoding='utf-8').writelines(self.__lines)

        self.__changed = False

    def load(self):
        self.__lines = open(self.__filename, 'r', encoding='utf-8').readlines()

    def first_line_matching(self, match_function):
        index = 0
        for line in self.__lines:
            if match_function(line):
                return index

            index += 1

        return -1

    def previous_line_matching(self, index, match_function):
        index -= 1
        while index >= 0:
            if match_function(self.__lines[index]):
                return index
            index -= 1

        return -1

    def next_line_matching(self, index, match_function):
        index += 1
        while index < len(self.__lines):
            if match_function(self.__lines[index]):
                return index
            index += 1

        return -1

    def remove_lines_by_index(self, lines_to_remove):
        offset = 0
        for index in sorted(lines_to_remove):
            self.__lines = self.__lines[:index - offset] + self.__lines[index + 1 - offset:]
            offset += 1

        self.__changed = True

    def insert_line_at_index(self, line, index):
        self.__lines.insert(index, line)
        self.__changed = True

    def remove_lines_matching(self, match_function):
        lines_to_remove = []
        for index in range(0, len(self.__lines)):
            if match_function(self.__lines[index]):
                lines_to_remove.append(index)

        if len(lines_to_remove) > 0:
            self.remove_lines_by_index(lines_to_remove)

    def replace(self, pattern, replace):
        for i in range(0, len(self.__lines)):
            updated = self.__lines[i].replace(pattern, replace)
            if updated != self.__lines[i]:
                self.__lines[i] = updated
                self.__changed = True
