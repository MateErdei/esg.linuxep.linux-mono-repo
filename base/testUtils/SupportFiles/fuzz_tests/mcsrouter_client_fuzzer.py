from kitty.fuzzers import ClientFuzzer
import datetime


class McsRouterClientFuzzer(ClientFuzzer):
    def __init__(self, name='mcsrouter-fuzzer', logger=None, option_line=None):
        super(McsRouterClientFuzzer, self).__init__(name, logger, option_line)
        self.last_served_payloads = {}
        self.failed_policies_file = None

    def _store_report(self, report):
        '''
        Override to store the last served policies before a failure
        :param report:
        :return:
        '''
        super(McsRouterClientFuzzer, self)._store_report(report)
        if report.get_status() == 'failed':
            template_stage = self.model.get_template_info()['name']
            test_num = self.model.current_index()
            time_stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
            self.failed_policies_file = \
                "./kittylogs/last_served_policies_template-{}_test-{}_{}.json".format(template_stage, test_num,
                                                                                      time_stamp)
            with open(self.failed_policies_file, 'w') as fp:
                for key, val in self.last_served_payloads.items():
                    fp.write(key)
                    fp.write('\n')
                    for item in val.data:
                        fp.write(item)
                        fp.write('\n')

    def get_template_stage_name(self):
        return self.model.get_template_info()['name']

    def update_fuzzed_policies(self, stage, policy):
        if stage in self.last_served_payloads:
            self.last_served_payloads[stage].append(policy)
        else:
            rb = RingBuffer(3)
            rb.append(policy)
            self.last_served_payloads[stage] = rb

    def _should_fuzz_node(self, fuzz_node, stage):
        #Hacking  hould_fuzz_node(fuzz_node, stage) so we can ensure the type of stage is byte
        stage_as_str = stage.decode()
        return super(McsRouterClientFuzzer, self)._should_fuzz_node(fuzz_node, stage_as_str)


class RingBuffer(object):
    """ class that implements a not-yet-full buffer """

    def __init__(self, size_max):
        self.max = size_max
        self.data = []

    class __Full(object):
        """ class that implements a full buffer """

        def append(self, x):
            """ Append an element overwriting the oldest one. """
            self.data[self.cur] = x
            self.cur = (self.cur + 1) % self.max

        def get(self):
            """ return list of elements in correct order """
            return self.data[self.cur:] + self.data[:self.cur]

    def append(self, x):
        """append an element at the end of the buffer"""
        self.data.append(x)
        if len(self.data) == self.max:
            self.cur = 0
            # Permanently change self's class from non-full to full
            self.__class__ = self.__Full

    def get(self):
        """ Return a list of elements from the oldest to the newest. """
        return self.data


# sample usage
if __name__ == '__main__':
    x = RingBuffer(5)
    x.append(1)
    x.append(2)
    x.append(3)
    x.append(4)
    print(x.get())
    x.append(7)
    x.append(8)
    x.append(9)
    x.append(10)
    print(x.data)

    dic = {'test': x}
    for key, val in dic.items():
        print(key)
        print('\n')
        for item in val.data:
            print(item)
            print('\n')
