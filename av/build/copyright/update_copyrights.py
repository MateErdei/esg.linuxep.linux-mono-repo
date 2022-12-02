# Copyright 2022 Sophos Limited. All rights reserved.
"""
Update the copyright header in a file to the current date.
"""
import datetime
import os
import re


COPYRIGHT_RE = re.compile(r'.* Copyright ((\d{4})|(\d{4})\-(\d{4})) Sophos Limited\. All rights reserved\.')
CURRENT_YEAR = str(datetime.datetime.now().year)
COMMENT_TOKENS = {
    ".c": "// ", ".cpp": "// ", ".h": "// ", ".hpp": "// ",
    ".py": "# ",
    ".lua": "-- ",
    "BUILD": "# ", ".bazel": "# ", ".bzl": "# "
}
SINGLE_YEAR_COMMENT = "Copyright %s Sophos Limited. All rights reserved.\n"
MULTI_YEAR_COMMENT = "Copyright %s-%s Sophos Limited. All rights reserved.\n"

SUPPORTED_FILE_TYPES = ['*' + ext for ext in COMMENT_TOKENS]


def run(code_file):
    split = os.path.splitext(code_file.filename())
    pattern = split[1]
    if not pattern:
        pattern = os.path.basename(split[0])
    comment_token = COMMENT_TOKENS[pattern]
    lines = code_file.lines()

    copyright_index = 0
    if lines[0][:2] == '#!':
        copyright_index = 1

    # Remove old-style copyright headers
    if lines[0].startswith("//===="):
        index = 1
        while not lines[index].startswith("//===="):
            index += 1

        lines = [lines[1]] + lines[index + 1:]
        code_file.update(lines)

    match = COPYRIGHT_RE.match(lines[copyright_index])
    if not match:
        # No header. If this has already been committed and there's an initial year,
        # use that. Otherwise use the current year.
        initial_year = CURRENT_YEAR
        try:
            initial_commit = code_file.git_repo().iter_commits(
                date_order=True, reverse=True, paths=code_file.git_path()).__next__()
            initial_year = initial_commit.authored_datetime.year
        except:  # pylint: disable=bare-except
            pass

        if initial_year == CURRENT_YEAR:
            lines.insert(0, comment_token + SINGLE_YEAR_COMMENT % CURRENT_YEAR)
        else:
            lines.insert(0, comment_token + MULTI_YEAR_COMMENT % (initial_year, CURRENT_YEAR))

        code_file.update(lines)

    elif match.groups(0)[1] != 0:
        # Single year header. Update if current year is different than the year
        # in the copyright.
        comment_year = match.groups(0)[1]
        if comment_year != CURRENT_YEAR:
            lines[copyright_index] = comment_token + MULTI_YEAR_COMMENT % (comment_year, CURRENT_YEAR)
            code_file.update(lines)

    else:
        # Multi-year header. Update if the current year is different than the
        # last year of the copyright.
        start_year = match.groups(0)[2]
        end_year = match.groups(0)[3]
        if end_year != CURRENT_YEAR:
            lines[copyright_index] = comment_token + MULTI_YEAR_COMMENT % (start_year, CURRENT_YEAR)
            code_file.update(lines)
