#!usr/bin/env python

"""
Module docstring.
"""

import json
import logging
import optparse
import os
import stat
import sys
from xml.etree import ElementTree

from xml.dom import minidom
import yaml

import components
import locations
import warehouses


# TODO: remove this funcdef - only for developement


def prettify(elem):
    """Return a pretty-printed XML string for the Element.
    """
    rough_string = ElementTree.tostring(elem, 'utf-8')
    reparsed = minidom.parseString(rough_string)
    return reparsed.toprettyxml(indent="  ")


def make_component_core(c):
    name = c['instance-name']
    logging.debug("Making component %s", name)
    line_id = c['line-id']
    canonical_name = c['canonical-name']
    if 'fileset' in c:
        fileset = c['fileset']
        return components.Component(name, line_id, canonical_name, fileset=fileset)
    else:
        build_spec = components.ComponentBuild(c['build'])
        return components.Component(name, line_id, canonical_name, build_spec=build_spec)


def update_component_suite_version(path, description, version):
    sdds_import_file_path = os.path.join(path, "SDDS-Import.xml")
    import_spec = ElementTree.parse(sdds_import_file_path)
    import_spec.find(".//Version").text = version
    import_spec.find(".//ShortDesc").text = description
    import_spec.find(".//LongDesc").text = description
    import_spec.find(".//Token").text = "{}#{}".format(import_spec.find(".//RigidName").text, version)
    os.chmod(sdds_import_file_path, stat.S_IWRITE)
    import_spec.write(sdds_import_file_path)


def make_component(c):
    comp = make_component_core(c)
    if 'mountpoint' in c:
        comp.set_mountpoint(c['mountpoint'])
    if 'tlcname' in c:
        comp.set_tlc(c['tlcname'])
    if 'features' in c:
        comp.set_features(c['features'])
    if 'platforms' in c:
        comp.set_platforms(c['platforms'])
    if 'subcomponents' in c:
        logging.debug("Setting subcomponents")
        comp.set_subcomponents(c['subcomponents'])
    if 'import' in c:
        update_component_suite_version(comp.path, c['import']['description'], c['import']['version'])
    return comp


def check_components(parsed_input):
    for c in parsed_input['components']:
        if 'fileset' not in c:
            print(components.ComponentBuild(c['build']))


def make_components(parsed_input):
    comps = dict()
    for c in parsed_input['components']:
        comp = make_component(c)
        comps[comp.name] = comp
    return comps


def resolve_subcomponents(comps):
    index = {}
    for c in comps.values():
        index[c.name] = (c.line_id, c.version, c.mountpoint)
    for c in comps.values():
        c.resolve_subcomponents(index)


def publish_components(parsed_input, whouses, components):
    for c in parsed_input['components']:
        name = c['instance-name']

        logging.debug("Publishing %s", name)
        for w in c['warehouses']:
            logging.debug("To %s", w['id'])
            item = warehouses.WarehouseItem(
                c['line-id'], components[name].version)
            if 'promotional-state' in w:
                logging.debug("with promotional state %s", w['promotional-state'])
                item.set_promotional_state(w['promotional-state'])
            if 'release-tags' in w:
                item.set_release_tags(w['release-tags'])
            if 'resubscription' in w:
                goto_name = w['resubscription']['instance-name']
                logging.debug("with resubscription to %s", goto_name)
                base_version = w['resubscription']['base-version']
                line_id = components[goto_name].line_id
                version = components[goto_name].version
                item.set_resubscription(line_id, base_version, version)
            for s in w.get('supplements', []):
                logging.info("Adding supplement {} to {}".format(s['warehouse'], w['id']))
                supp = warehouses.Supplement(
                    s['warehouse'],
                    s['line-id'],
                    s['tag'])
                if 'base-version' in s:
                    supp.set_base_version(s['base-version'])
                if 'decode-path' in s:
                    supp.set_decode_path(s['decode-path'])
                item.add_supplement(supp)
            whouses[w['id']].add_item(item)


def make_warehouses(parsed_input):
    warehouse_list = {}
    for c in parsed_input['components']:
        logging.debug("Finding warehouses for %s", c['instance-name'])
        for w in c['warehouses']:
            if w['id'] not in warehouse_list:
                logging.debug("Adding %s to list of warehouses", w['id'])
                warehouse_list[w['id']] = warehouses.Warehouse(
                    w['id'], w['type'])
    return warehouse_list


def build_location_element(options):
    location = ElementTree.Element("Location")
    ElementTree.SubElement(location, "WriteTo",
                           path=locations.WAREHOUSE_WRITE)
    read_from = ElementTree.SubElement(location, "ReadFrom")
    for s in locations.WAREHOUSE_READ:
        ElementTree.SubElement(read_from, "URL", path=s)
    return location


def build_element_tree(comps, whouses, options):
    index = dict()
    for c in comps.values():
        index.setdefault(c.line_id, [])
        index[c.line_id].append(c)
    # render the objects as an xml Element Tree
    root = ElementTree.Element("PublicationsDefinition")
    assemblies = ElementTree.SubElement(root, "Assemblies")
    for line_id, line in index.items():
        logging.debug("Rendering {}".format(line_id))
        line_element = ElementTree.SubElement(assemblies, "Line", id=line_id)
        ElementTree.SubElement(
            line_element, "CanonicalName", name=line[0].canonical_name)
        instances = ElementTree.SubElement(line_element, "Instances")
        for c in line:
            logging.debug("Rendering instance {}".format(c._name))
            instances.append(c.to_element_tree())
    pubs = ElementTree.SubElement(root, "Publications")
    pub = ElementTree.SubElement(pubs,
                                 "Publication",
                                 id="dev")
    repositories = ElementTree.SubElement(pub, "Repositories")
    repository = ElementTree.SubElement(
        repositories, "Repository", id=locations.REPOSITORY_ID)
    location = build_location_element(options)
    repository.append(location)
    warehouses = ElementTree.SubElement(repository, "Warehouses")
    for key in whouses:
        warehouses.append(whouses[key].to_element_tree())
    return root


def write_bom(f, comps, options):
    bom_components = []
    for comp_name in comps:
        build_spec = comps[comp_name]._build_spec
        logging.debug(comp_name)
        if build_spec:
            bom_components.append({"warehouse_component": comp_name,
                                   "build_name": build_spec.name,
                                   "version": comps[comp_name].version,
                                   })

    bom_build = {"branch": os.environ.get('SOURCE_CODE_BRANCH'),
                 "commit": os.environ.get('SOURCE_CODE_COMMIT_HASH'),
                 "jenkins_job_name": os.environ.get('JOB_NAME'),
                 "jenkins_branch_name": os.environ.get('BRANCH_NAME'),
                 "jenkins_build_number": os.environ.get('BUILD_NUMBER')}
    bom_trigger_build = {"warehouse_component": "",
                         "build_name":"",
                         "branch": "",
                         "version": "",
                         "source_id": "",
                         "build_id": "",
                         "jenkins_job_name": "",
                         "jenkins_branch_name": "",
                         "jenkins_build_number": ""}
    bom = {"build": bom_build,
           "components": bom_components,
           "trigger": bom_trigger_build}
    json.dump(bom, f, indent=4)

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
    parser.add_option("-m", "--bom", dest="bom",
                      help="Write bill-of-materials list and warehouse details to FILE",
                      metavar="FILE")
    parser.add_option('-v', '--verbose', action="store_true", dest='verbose')
    parser.add_option('--dry_run', action="store_true",
                      help="Check the filer references are resolvable (useful for pre-commit hook)")
    parser.add_option(      # customized description; put --help last
        '-h', '--help', action='help',
        help='Show this help message and exit.')

    options, args = parser.parse_args(argv)

    # check number of arguments, verify values, etc.:
    if len(args) != 1:
        parser.error('input file not specified')

    # further process settings & args if necessary

    return options, args


def main(argv=None):
    logging.basicConfig(format="%(levelname)s:%(asctime)s:%(message)s",
                        stream=sys.stdout)
    options, args = process_command_line(argv)
    if options.verbose:
        logging.getLogger('').setLevel(logging.DEBUG)

    logging.info("STARTING warehouse-def.py")

    # read and parse the input file
    input_file = args[0]
    logging.debug("input file : {}".format(input_file))
    with open(input_file, 'r') as infile:
        yaml_input = infile.read()
        logging.debug("input : {}".format(yaml_input))

    model = yaml.safe_load(yaml_input)

    if options.dry_run:
        check_components(model)
        return
    # create component objects - one for each component instance that
    # will be imported
    comps = make_components(model)
    resolve_subcomponents(comps)

    # create the warehouse objects and publish components to them.
    whouses = make_warehouses(model)
    publish_components(model, whouses, comps)

    root_element = build_element_tree(comps, whouses, options)

    if options.outputFile is not None:
        folder = os.path.dirname(options.outputFile)
        if not folder == '' and not os.path.exists(folder):
            os.makedirs(folder)
        f = open(options.outputFile, "w")
    else:
        f = sys.stdout
    # bugfix: Element tree won't write out the xml tag if the encoding is utf-8
    # so write it out here
    logging.debug("Writing def file to {}".format(options.outputFile))
    f.write("<?xml version='1.0' encoding='utf-8'?>")
    f.write(ElementTree.tostring(root_element).decode('utf-8'))

    if options.bom is not None:
        folder = os.path.dirname(options.bom)
        if not folder == '' and not os.path.exists(folder):
            os.makedirs(folder)
        with open(options.bom, "w") as b:
            write_bom(b, comps, options)


    logging.info("FINISHING warehouse-def.py")
    return 0        # success


if __name__ == '__main__':
    status = main()
    sys.exit(status)
