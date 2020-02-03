import time
from kitty.targets.client import ClientTarget
import json
import base64
import datetime


class McsRouterTarget(ClientTarget):
    def __init__(self, name="McsClientTarget", logger=None):
        super(McsRouterTarget, self).__init__(name, logger)

    def trigger(self):
        '''
        Trigger the target (e.g. the victim application) to start communication with the fuzzer.
        '''
        assert (self.controller)
        self._trigger()
        self.logger.debug('Waiting for mutation response. (timeout = %d)'
                          % self.mutation_server_timeout)
        res = self.response_sent_event.wait(self.mutation_server_timeout)
        if not res:
            self.logger.warning('Warning: trigger timed out')
        else:
            time.sleep(self.post_fuzz_delay)
        self.response_sent_event.clear()
