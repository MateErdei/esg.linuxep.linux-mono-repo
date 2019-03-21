#!/usr/bin/env python


class IConnection(object):
    def action_completed(self, action):
        pass

    def status_update(self, status):
        pass
