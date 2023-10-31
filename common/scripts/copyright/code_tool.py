# Copyright 2022 Sophos Limited. All rights reserved.
import argparse
import glob
import importlib
import itertools
import os

import git

from code_file import CodeFile


def branch_generator(repo):
    """
    Get all files changed between this branch and develop
    :param repo:
    :return:
    """
    return itertools.chain(
        (os.path.join(repo.working_dir, x.b_path) for x in repo.index.diff("develop") if not x.deleted_file),
        (os.path.join(repo.working_dir, x) for x in repo.untracked_files))


def changed_generator(repo):
    return itertools.chain(
        (os.path.join(repo.working_dir, x.b_path) for x in repo.index.diff(None) if not x.deleted_file),
        (os.path.join(repo.working_dir, x) for x in repo.untracked_files))


def staged_generator(repo):
    return (os.path.join(repo.working_dir, x.b_path) for x in repo.index.diff("HEAD") if not x.deleted_file)


def folder_generator(repo, folders):
    # Get the generator for a given folder. Need an inner method here because
    # we want the generator for a folder to use the values of abs_path and
    # relative_path that correspond to the folder.
    def _folder_generator(repo, folder):
        abs_path = os.path.abspath(folder)
        relative_path = abs_path[len(repo.working_dir) + 1:].replace('\\', '/') + '/'
        return (
            os.path.join(repo.working_dir, x[0]) for x in repo.index.entries.keys() if x[0].startswith(relative_path))

    generators = [_folder_generator(repo, folder) for folder in folders]
    return itertools.chain(*generators)


def file_is_supported(rule, filepath):
    supported_types = rule.SUPPORTED_FILE_TYPES
    if not supported_types:
        return True

    filename = os.path.basename(filepath)
    for filetype in supported_types:
        if glob.fnmatch.fnmatch(filename, filetype):
            return True

    return False


def run_rules(git_repo, generator, rules, verbose):
    total_files = 0
    changed_files = 0

    for filename in generator:
        total_files += 1
        for rule in rules:
            try:
                if not file_is_supported(rule, filename):
                    if verbose:
                        print(f"Skipping {rule.__name__} on {filename}, file type not supported.")
                    continue

                if verbose:
                    print(f"Running {rule.__name__} on {filename}.")

                file = CodeFile(git_repo, filename)
                rule.run(file)
                if file.updated():
                    file.save()
                    if verbose:
                        print("    Changed.")
                    changed_files += 1

            except Exception as exc:  # pylint: disable=broad-except
                print(f"Rule {rule.__name__} failed to process {filename} with error: {exc}")

    print(f"Processed {total_files} files, changed {changed_files}.")


def get_git_folder(folder, verbose):
    repo_folder = os.getcwd() if folder is None else folder
    while True:
        if verbose:
            print(f"Checking for repo in folder {repo_folder}")
        if os.path.exists(os.path.join(repo_folder, '.git')):
            return repo_folder

        parent_folder = os.path.dirname(repo_folder)
        if parent_folder == repo_folder:
            raise Exception(f"No git folder found in {folder}")

        repo_folder = parent_folder


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--verbose", action="store_true")
    filegroup = parser.add_mutually_exclusive_group()
    filegroup.add_argument("--branch", action="store_true")
    filegroup.add_argument("--changed", action="store_true")
    filegroup.add_argument("--changes", action="store_true")
    filegroup.add_argument("--staged", action="store_true")
    filegroup.add_argument("--folder", type=str, dest='folders', action='append')
    parser.add_argument("rule_names", nargs="+")

    args = parser.parse_args()

    verbose = args.verbose
    git_folder = os.path.abspath(args.folders[0]) if args.folders is not None else None
    git_repo = git.Repo(get_git_folder(git_folder, verbose))
    if args.changed or args.changes:
        generator = changed_generator(git_repo)
    elif args.staged:
        generator = staged_generator(git_repo)
    elif args.branch:
        generator = branch_generator(git_repo)
    else:
        generator = folder_generator(git_repo, args.folders)

    rules = []
    for rule_name in args.rule_names:
        if verbose:
            print(f"Loading rule {rule_name}")
        rule_module = importlib.import_module(rule_name.replace('-', '_'))
        rules.append(rule_module)

    run_rules(git_repo, generator, rules, verbose)


if __name__ == "__main__":
    main()
