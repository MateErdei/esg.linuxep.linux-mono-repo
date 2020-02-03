import mcsrouter_policy_templates as mcs_templates
from kitty.model import Template

from katnip.legos.xml import XmlElement, XmlAttribute

'''
use file to prepare a verify templates content from 'mcsrouter_policy_templates.py'
ref:https://katnip.readthedocs.io/en/stable/katnip.legos.xml.html
'''



if __name__ == '__main__':
    # Attribute defaults
    # name, attribute, value, fuzz_attribute=False, fuzz_value=True

    # XmlElement defaults
    # name, element_name, attributes, contenet, fuzz_name=True, fuzz_content=False

    # head = '<?xml version="1.0"?>\n'
    test_mcs_template = mcs_templates.mcs_policy_fuzzed
    test_mdr_template = mcs_templates.mdr_policy_fuzzed
    test_alc_template = mcs_templates.alc_policy_fuzzed

    print("mdr  template")
    print(test_mdr_template.render().tobytes().decode())
    print("\n\n mcs template")
    print(test_mcs_template.render().tobytes().decode())
    print("\n\n alc template")
    print(test_alc_template.render().tobytes().decode())
