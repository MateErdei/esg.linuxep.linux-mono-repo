import os
import pwd
import grp


def create_group(group):
    if group not in grp.getgrall():
        os.system('groupadd {}'.format(group))


def create_user(user):
    if user not in pwd.getpwall():
        os.system('useradd {}'.format(user))


def create_users_and_group():
    create_user('sophos-spl-user')
    create_group('sophos-spl-group')
