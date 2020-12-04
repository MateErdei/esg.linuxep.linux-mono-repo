import logging
import optparse
import os
import sys
import yaml


class DefinitionMerger:
    def __init__(self):
        self.components = {}
        self.customer_files = []
        self.common_component_data = yaml.safe_load(open(r'..\def\common\components.yaml').read())
        self.common_supplement_data = yaml.safe_load(open(r'..\def\common\supplements.yaml').read())


    def merge(self, input):
        input_model = yaml.safe_load(input)
        self.customer_files += input_model['customer-files']
        for wh_name, wh_data in input_model['warehouses'].items():
            for comp_name, comp_data in wh_data['components'].items():
                self.process_input_component(wh_name, comp_name, comp_data)

    def process_input_component(self, wh_name, comp_name, comp_data):
        comp_data = comp_data or {}

        if not comp_name in self.components:
            self.components[comp_name] = self.common_component_data[comp_name]
            self.components[comp_name].update({
                'instance-name': comp_name,
                'warehouses': []
            })
        component = self.components[comp_name]

        if 'subcomponents' in comp_data:
            subcomponents = comp_data['subcomponents'] or {}
            component['subcomponents'] = list(subcomponents.keys())
            for subcomp_name, subcomp_data in subcomponents.items():
                self.process_input_component(wh_name, subcomp_name, subcomp_data)
        if 'import' in comp_data:
            component['import'] = comp_data['import']
        if 'features' in comp_data:
            component['features'] = comp_data['features']
        if 'platforms' in comp_data:
            component['platforms'] = comp_data['platforms']

        warehouse = {
            'id': wh_name,
            'type': 'sdds2'
        }
        if 'release-tags' in comp_data:
            warehouse['release-tags'] = comp_data['release-tags']
        if 'supplements' in comp_data:
            supplements = []
            for supp_id in comp_data['supplements']:
                assert not isinstance(supp_id, dict), "Got a supp_id that is a dictionary!"
                supplements.append(self.common_supplement_data[supp_id])
            warehouse['supplements'] = supplements
        component['warehouses'].append(warehouse)

    def output(self):
        return yaml.dump({
            'customer-files': self.customer_files,
            'components': list(self.components.values())
        }, indent=4, default_flow_style=False)


def process_command_line(argv):
    """
    Return a 2-tuple: (settings object, args list).
    `argv` is a list of arguments, or `None` for ``scs.argv[1:]``.
    """
    if argv is None:
        argv = sys.argv[1:]

    # initialize the parser object:
    parser = optparse.OptionParser(
        formatter=optparse.TitledHelpFormatter(width=78),
        add_help_option=None)

    # define options here:
    parser.add_option("-o", "--output", dest="outputFile",
                      help="Write output to FILE", metavar="FILE")
    parser.add_option('-v', '--verbose', action="store_true", dest='verbose')
    parser.add_option(      # customized description; put --help last
        '-h', '--help', action='help',
        help='Show this help message and exit.')

    options, args = parser.parse_args(argv)

    # check number of arguments, verify values, etc.:
    if len(args) < 1:
        parser.error('no input files were specified')

    return options, args


def merge(output_path, input_files):
    merger = DefinitionMerger()
    for input_file in input_files:
        logging.info("Merging def file {}".format(input_file))
        merger.merge(open(input_file).read())

    if output_path is not None:
        folder = os.path.dirname(output_path)
        if not folder == '' and not os.path.exists(folder):
            os.makedirs(folder)
        logging.debug("Writing def file to {}".format(output_path))
        with open(output_path, "w") as f:
            f.write(merger.output())
    else:
        print(merger.output())


def main(argv=None):
    logging.basicConfig(format="%(levelname)s:%(asctime)s:%(message)s",
                        stream=sys.stdout)
    options, args = process_command_line(argv)
    if options.verbose:
        logging.getLogger('').setLevel(logging.DEBUG)

    merge(options.outputFile)
    return 0        # success


if __name__ == '__main__':
    status = main()
    sys.exit(status)
