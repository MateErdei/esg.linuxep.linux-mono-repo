
import os
import pwd
import grp

class UserUtils():
    def create_user(self, user):
        if user not in pwd.getpwall():
            os.system('useradd {}'.format(user))


    def create_group(self, group):
        if group not in grp.getgrall():
            os.system('groupadd {}'.format(group))


    def create_users_and_group(self):
        self.create_user('sophos-spl-user')
        self.create_group('sophos-spl-group')
