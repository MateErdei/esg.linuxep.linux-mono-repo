import os
import yaml
import json
import copy
import shutil

import xml.etree.ElementTree as ET
from common import visit_suite_instance_views, visit_suite_instances, visit_instance_views
from common import get_absolute_fileset, is_static_suite_instance
from common import hash_file, say
TOOLS = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.dirname(TOOLS)
ROOT = os.path.dirname(BASE)
STATIC_FLAG_OVERRIDES = yaml.safe_load(open(fr'{ROOT}/def-sdds3/static_flag_overrides.yaml'))
ALL_STATIC_FLAGS = {}
def _expand_static_suites(suites):
    for _, suitedef, instance in visit_suite_instances(suites):
        if 'generate_static' not in instance or not instance['generate_static']:
            continue
        del instance['generate_static']

        copyinst = copy.deepcopy(instance)
        copyinst['tags'] = ['_STATIC']
        suitedef['instances'].append(copyinst)


def _generate_static_suite_flags(flagsfile, view, common_component_data, common_supplements_data):
    if 'supplements' not in view:
        return False

    generated = False
    supplements = []
    for suplink in view['supplements']:
        if 'line-id' not in suplink or suplink['line-id'] != 'SSPLFLAGS':
            supplements.append(suplink)
            continue

        fileset = _find_supplement_fileset(common_component_data, common_supplements_data,
                                           suplink['line-id'], suplink['tag'])
        if not fileset:
            raise NameError(f'Did not find supplement fileset for {suplink}')

        basepath = get_absolute_fileset(fileset)

        for f in _each_file_in_sdds_import(fileset):
            entry = _get_file_package_content_entry_from_attrib(f.attrib)

            # Skip the manifest.dat file, because we're modifying the file!
            if entry['path'].endswith('manifest.dat'):
                continue

            source = os.path.join(basepath, entry['path'])
            with open(source) as f:
                flagdata = json.load(f)
            for flag in flagdata:
                flagval = flagdata[flag]
                if flagval not in ['always', 'always-after-reboot']:
                    flagdata[flag] = False
            _apply_static_flag_overrides(suplink['line-id'], flagdata)

            with open(flagsfile, 'w') as f:
                json.dump(flagdata, f)

            entry['sha256'] = hash_file(flagsfile)
            entry['path'] = '/'.join(['@flags', entry['path']])
            generated = True
    view['supplements'] = supplements
    return generated

def _import_static_suite_flags(compdef, filelist, abs_fileset):
    if 'static_suite_flags' not in compdef:
        return
    for tgt, src in compdef['static_suite_flags'].items():
        target = os.path.join(abs_fileset, tgt)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        say(f'Importing static flags {src} -> {target}')
        shutil.copyfile(src, target)

        # Insert into SDDS-Import.xml
        elem = ET.SubElement(filelist, 'File')
        elem.attrib['Name'] = os.path.basename(tgt)
        if '/' in tgt:
            elem.attrib['Offset'] = tgt[:tgt.rfind('/')]

        # Insert the Sha256, to ensure that different flags produce different SDDS-Import.xml files,
        # so that those in turn produce different nonces for the static suites.
        elem.attrib['Sha256'] = hash_file(target)


def _find_supplement_fileset(common_component_data, supplements, lineid, tag):
    for compid in supplements['supplements'][lineid]['components']:
        supdef = supplements['supplements'][lineid]['components'][compid]
        for suptag in supdef['release-tags']:
            if tag == suptag['tag']:
                return common_component_data[compid]['fileset']
    raise NameError(f'Could not find fileset for supplement {lineid} tag {tag}')


def _each_file_in_sdds_import(fileset):
    sdds_import = os.path.join(get_absolute_fileset(fileset), 'SDDS-Import.xml')
    with open(sdds_import) as f:
        xml = ET.fromstring(f.read())
    for f in xml.findall('Component/FileList/File'):
        yield f


def _apply_static_flag_overrides(lineid, flagsdata):
    if STATIC_FLAG_OVERRIDES and lineid in STATIC_FLAG_OVERRIDES:
        for flag in STATIC_FLAG_OVERRIDES[lineid]:
            flagsdata[flag] = STATIC_FLAG_OVERRIDES[lineid][flag]
    ALL_STATIC_FLAGS[lineid] = flagsdata


def _get_file_package_content_entry_from_attrib(attrib):
    entry = {}
    if 'Offset' in attrib:
        entry['path'] = '/'.join([attrib['Offset'], attrib['Name']])
    else:
        entry['path'] = attrib['Name']
    if 'MD5' in attrib:
        entry['MD5'] = attrib['MD5']
    return entry
