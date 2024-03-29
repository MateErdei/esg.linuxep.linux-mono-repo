# -*- coding: utf-8 -*-
import sys
import replace_types
#this is a hack to satisfy katnip.legos.xml which is 'broken' for python3 (23.01.20)
sys.modules['types'] = sys.modules['replace_types']

from katnip.legos.xml import XmlElement, XmlAttribute
from kitty.model import Delimiter, Static, String, Template

### common fileds ####
# declaration does not fit kitty xmlelement so its defined with basic types here outside
# defaults String fuzzable=True    &&   Static means static

declaration_static = Static("<?xml version='1.0'?>", name="declaration")
declaration_fuzz = String("<?xml version='1.0'?>", name="declaration")
newline_static = Static("\n", name="newline")

#   XmlAttribute defaults -> fuzz_attribute=False, fuzz_value=True
#   XmlElement defaults -> fuzz_name=True, fuzz_content=False

### mcs policy ###
# actively fuzz the whole policy and its attributes
config_element = \
    XmlElement(name="config_element", attributes=[
        XmlAttribute(name="xmlns", attribute="xmlns",
                     value="http://www.sophos.com/xml/msys/mcspolicy.xsd", fuzz_attribute=True,
                     fuzz_value=True),
        XmlAttribute(name="auto-nsl", attribute="xmlns:auto-ns1",
                     value="com.sophos\mansys\policy", fuzz_attribute=True, fuzz_value=True)],
               element_name="configuration",content=[
            XmlElement(name="reg_token", element_name="registrationToken", content="PolicyRegToken",
                       delimiter="\n", fuzz_name=True, fuzz_content=False),
            XmlElement(name="customer_id", element_name="customerId", content="999ff111-2a49-e452-7676-3105612ba3a3",
                       delimiter="\n", fuzz_name=True, fuzz_content=False),
            XmlElement(name="servers", element_name="servers", content=[
                XmlElement(name="server", element_name="server",
                           content="https://localhost:4443/mcs", fuzz_name=True, fuzz_content=False,
                           delimiter="\n")], fuzz_name=True, fuzz_content=False, delimiter="\n"),

            XmlElement(name="proxies", element_name="proxies", content=[
                XmlElement(name="proxy", element_name="proxy",
                           content="http://192.168.36.37:3129",
                           fuzz_name=True, fuzz_content=False, delimiter="\n")],
                       fuzz_name=True, fuzz_content=True, delimiter="\n"),
            XmlElement(name="msg_relays", element_name="messageRelays", fuzz_name=True,
                       fuzz_content=False, delimiter="\n"),
            XmlElement(name="proxy_credentials", element_name="proxyCredentials", content=[
                XmlElement(name="credentials", element_name="credentials",
                           content="KHDFWN27975-FKDNDD+JFJ/REI7235-OJ=",
                           fuzz_name=True, fuzz_content=False, delimiter="\n")],
                       fuzz_name=True, fuzz_content=True, delimiter="\n"),

                   XmlElement(name="use_sys_proxy", element_name="useSystemProxy", content="true",
                              fuzz_name=True, fuzz_content=True, delimiter="\n"),
                   XmlElement(name="use_auto_proxy", element_name="useAutomaticProxy", content="true",
                              fuzz_name=True, fuzz_content=False, delimiter="\n"),
                   XmlElement(name="use_direct", element_name="useDirect", content="true",
                              fuzz_name=True, fuzz_content=True, delimiter="\n"),
                   XmlElement(name="rand_skew_factor", element_name="randomSkewFactor", content=1,
                              fuzz_name=True, fuzz_content=True, delimiter="\n"),
                   XmlElement(name="cmd_polling_delay", element_name="commandPollingDelay", attributes=[
                       XmlAttribute(name="default", attribute="default", value="1", fuzz_attribute=False,
                                    fuzz_value=False)],
                              fuzz_name=True, fuzz_content=True, delimiter="\n"),

                   XmlElement(name="flags_polling_interval", element_name="flagsPollingInterval", attributes=[
                        XmlAttribute(name="default", attribute="default", value="14400", fuzz_attribute=True,
                                     fuzz_value=True)],
                               fuzz_name=True, fuzz_content=True, delimiter="\n"),

                   XmlElement(name="policy_change_servers", element_name="policyChangeServers",
                              fuzz_name=True, fuzz_content=True, delimiter="\n"),
                   XmlElement(name="presigned_url_service", element_name="presignedUrlService", content=[
                       XmlElement(name="url", element_name="url", content="https://localhost:4443/mcs/sophos/management/ep/presignedurls",
                                  fuzz_name=True, fuzz_content=False, delimiter="\n"),
                       XmlElement(name="credentials", element_name="credentials", content="KHDFWN27975-FKDNDD+JFJ/REI7235-OJ=",
                                  fuzz_name=True, fuzz_content=False, delimiter="\n")],
                              fuzz_name=True, fuzz_content=True, delimiter="\n"),

                   XmlElement(name="push_servers", element_name="pushServers", content=[
                       XmlElement(name="push_server", element_name="push_server", content="https://localhost:4443/mcs",
                                  fuzz_name=True, fuzz_content=False, delimiter="\n")],
                              fuzz_name=True, fuzz_content=True, delimiter="\n"),
                   XmlElement(name="push_ping_timeout", element_name="pushPingTimeout",
                              content=12, fuzz_name=True, fuzz_content=False, delimiter="\n"),
                   XmlElement(name="push_fallback_poll_interval", element_name="pushFallbackPollInterval", content=1,
                              fuzz_name=True, fuzz_content=False, delimiter="\n")
               ], fuzz_name=True, fuzz_content=True, delimiter="\n")

mcs_inner_elements = \
    [
        XmlElement(name='meta', element_name='meta',
                   attributes=[
                       XmlAttribute(name='protoVer', attribute='protocolVersion', value="1.1", fuzz_attribute=True,
                                    fuzz_value=True)
                   ], fuzz_name=True, fuzz_content=True, delimiter='\n'),
        XmlElement(name='comp', element_name='csc:Comp',
                   attributes=[
                       XmlAttribute(name='recid', attribute='RevID',
                                    value="0e09e27cfc21f8d7510d562c56e34507711bdce48bfdfd3846965f130fef142a",
                                    fuzz_attribute=True, fuzz_value=True),
                       XmlAttribute(name='policy_type', attribute='policyType', value='25', fuzz_attribute=True,
                                    fuzz_value=True),
                   ], fuzz_name=True, fuzz_content=True, delimiter='\n'),
        config_element
    ]

mcs_policy_attr = \
    [
        XmlAttribute(name='xmlns', attribute='xmlns:csc', value='com.sophos\msys\csc', fuzz_attribute=True,
                     fuzz_value=True),
        XmlAttribute(name='type', attribute='type', value='mcs', fuzz_attribute=True, fuzz_value=True)
    ]

mcs_policy = \
    XmlElement(name='fuzz_policy', element_name='policy', attributes=mcs_policy_attr, content=mcs_inner_elements,
               fuzz_content=True, fuzz_name=False, delimiter='\n')

mcs_policy_fuzzed = Template(fields=[declaration_static, newline_static, mcs_policy], name='mcs_policy_fuzz')


### alc policy ###
# actively fuzze "AUConfigurations" element as its the component processed by MCSRouter python code
alc_policy = \
    XmlElement(name='AUConfigs', element_name='AUConfigurations',
               attributes=[
                   XmlAttribute(name="xsi", attribute="xmlns:xsi", value="http://www.w3.org/2001/XMLSchema-instance", fuzz_attribute=True, fuzz_value=True),
                   XmlAttribute(name="xsd", attribute="xmlns:xsd", value="http://www.w3.org/2001/XMLSchema", fuzz_attribute=True, fuzz_value=True),
                   XmlAttribute(name="csc", attribute="xmlns:csc", value="com.sophos\msys\csc", fuzz_attribute=True, fuzz_value=True),
                   XmlAttribute(name="xmlns", attribute="xmlns", value="http://www.sophos.com/EE/AUConfig", fuzz_attribute=True, fuzz_value=True)
               ],
               content=[
                   XmlElement(name="csc-comp", element_name="csc:Comp", attributes=[
                       XmlAttribute(name='revid', attribute='RevID',
                                    value="5514d7970027ac81cc3686799c9359043dafbc72d4b809490ca82bacc4bf5026", fuzz_attribute=True, fuzz_value=True),
                       XmlAttribute(name="policytype", attribute="policyType", value="1", fuzz_attribute=True, fuzz_value=True)
                   ], fuzz_content=True, fuzz_name=True,  delimiter="\n"),
                   XmlElement(name="auconfig", element_name="AUConfig",
                              attributes=[
                                  XmlAttribute(name='platform', attribute='platform', value="Linux")
                              ],
                              content=[
                                  XmlElement(name="sophos_address", element_name="sophos_address",
                                             attributes=[XmlAttribute(name="address", attribute="address",
                                                                      value="http://es-web.sophos.com/update")],
                                             delimiter="\n"),
                                  XmlElement(name="primary_location", element_name="primary_location",
                                             content=[
                                                 XmlElement(name="server", element_name="server",
                                                            attributes=[
                                                                XmlAttribute(name="BandwidthLimit",
                                                                             attribute="BandwidthLimit", value="256"),
                                                                XmlAttribute(name="AutoDial", attribute="AutoDial",
                                                                             value="false"),
                                                                XmlAttribute(name="Algorithm", attribute="Algorithm",
                                                                             value="Clear"),
                                                                XmlAttribute(name="UserPassword",
                                                                             attribute="UserPassword",
                                                                             value="xn28ddszs1q"),
                                                                XmlAttribute(name="UserName", attribute="UserName",
                                                                             value="CSP7I0S0GZZE"),
                                                                XmlAttribute(name="UseSophos", attribute="UseSophos",
                                                                             value="true"),
                                                                XmlAttribute(name="UseHttps", attribute="UseHttps",
                                                                             value="true"),
                                                                XmlAttribute(name="UseDelta", attribute="UseDelta",
                                                                             value="true"),
                                                                XmlAttribute(name="ConnectionAddress",
                                                                             attribute="ConnectionAddress", value=""),
                                                                XmlAttribute(name="AllowLocalConfig",
                                                                             attribute="AllowLocalConfig",
                                                                             value="false")

                                                            ], delimiter="\n"),
                                                 XmlElement(name="proxy", element_name="proxy",
                                                            attributes=[
                                                                XmlAttribute(name="ProxyType", attribute="ProxyType",
                                                                             value="0"),
                                                                XmlAttribute(name="ProxyUserPassword",
                                                                             attribute="ProxyUserPassword", value=""),
                                                                XmlAttribute(name="ProxyUserName",
                                                                             attribute="ProxyUserName", value=""),
                                                                XmlAttribute(name="ProxyPortNumber",
                                                                             attribute="ProxyPortNumber", value="0"),
                                                                XmlAttribute(name="ProxyAddress",
                                                                             attribute="ProxyAddress", value=""),
                                                                XmlAttribute(name="AllowLocalConfig",
                                                                             attribute="AllowLocalConfig",
                                                                             value="false")

                                                            ], delimiter="\n")
                                             ], delimiter="\n"),
                                  XmlElement(name="secondary_location", element_name="secondary_location",
                                             content=[
                                                 XmlElement(name="server", element_name="server",
                                                            attributes=[
                                                                XmlAttribute(name="BandwidthLimit",
                                                                             attribute="BandwidthLimit", value="256"),
                                                                XmlAttribute(name="AutoDial", attribute="AutoDial",
                                                                             value="false"),
                                                                XmlAttribute(name="Algorithm", attribute="Algorithm",
                                                                             value=""),
                                                                XmlAttribute(name="UserPassword",
                                                                             attribute="UserPassword", value=""),
                                                                XmlAttribute(name="UserName", attribute="UserName",
                                                                             value=""),
                                                                XmlAttribute(name="UseSophos", attribute="UseSophos",
                                                                             value="false"),
                                                                XmlAttribute(name="UseHttps", attribute="UseHttps",
                                                                             value="true"),
                                                                XmlAttribute(name="UseDelta", attribute="UseDelta",
                                                                             value="true"),
                                                                XmlAttribute(name="ConnectionAddress",
                                                                             attribute="ConnectionAddress", value=""),
                                                                XmlAttribute(name="AllowLocalConfig",
                                                                             attribute="AllowLocalConfig",
                                                                             value="false")

                                                            ], delimiter="\n"),
                                                 XmlElement(name="proxy", element_name="proxy",
                                                            attributes=[
                                                                XmlAttribute(name="ProxyType", attribute="ProxyType",
                                                                             value="0"),
                                                                XmlAttribute(name="ProxyUserPassword",
                                                                             attribute="ProxyUserPassword", value=""),
                                                                XmlAttribute(name="ProxyUserName",
                                                                             attribute="ProxyUserName", value=""),
                                                                XmlAttribute(name="ProxyPortNumber",
                                                                             attribute="ProxyPortNumber", value="0"),
                                                                XmlAttribute(name="ProxyAddress",
                                                                             attribute="ProxyAddress", value=""),
                                                                XmlAttribute(name="AllowLocalConfig",
                                                                             attribute="AllowLocalConfig",
                                                                             value="false")

                                                            ], delimiter="\n")
                                             ], delimiter="\n"),
                                  XmlElement(name="schedule", element_name="schedule",
                                             attributes=[
                                                 XmlAttribute(name="AllowLocalConfig", attribute="AllowLocalConfig",
                                                              value="false"),
                                                 XmlAttribute(name="SchedEnable", attribute="SchedEnable",
                                                              value="true"),
                                                 XmlAttribute(name="Frequency", attribute="Frequency", value="60"),
                                                 XmlAttribute(name="DetectDialUp", attribute="DetectDialUp",
                                                              value="false")
                                             ], delimiter="\n"),
                                  XmlElement(name="logging", element_name="logging",
                                             attributes=[
                                                 XmlAttribute(name="AllowLocalConfig", attribute="AllowLocalConfig",
                                                              value="false"),
                                                 XmlAttribute(name="LogLevel", attribute="LogLevel", value="50"),
                                                 XmlAttribute(name="LogEnable", attribute="LogEnable", value="true"),
                                                 XmlAttribute(name="MaxLogFileSize", attribute="MaxLogFileSize",
                                                              value="1")
                                             ], delimiter="\n"),
                                  XmlElement(name="bootstrap", element_name="bootstrap",
                                             attributes=[
                                                 XmlAttribute(name="Location", attribute="Location", value=""),
                                                 XmlAttribute(name="UsePrimaryServerAddress",
                                                              attribute="UsePrimaryServerAddress", value="true")
                                             ], delimiter="\n"),
                                  XmlElement(name="cloud_subscription", element_name="cloud_subscription",
                                             attributes=[
                                                 XmlAttribute(name="RigidName", attribute="RigidName",
                                                              value="ServerProtectionLinux-Base"),
                                                 XmlAttribute(name="Tag", attribute="Tag", value="RECOMMENDED")
                                             ], delimiter="\n"),
                                  XmlElement(name="cloud_subscriptions", element_name="cloud_subscriptions",
                                             content=[
                                                 XmlElement(name="subscription_base", element_name="subscription",
                                                            attributes=[
                                                                XmlAttribute(name="Id", attribute="Id", value="Base"),
                                                                XmlAttribute(name="RigidName", attribute="RigidName",
                                                                             value="ServerProtectionLinux-Base"),
                                                                XmlAttribute(name="Tag", attribute="Tag",
                                                                             value="RECOMMENDED")
                                                            ], content=[], delimiter="\n"),
                                             ], delimiter="\n"),
                                  XmlElement(name="delay_supplements", element_name="delay_supplements",
                                             attributes=[
                                                 XmlAttribute(name="enabled", attribute="enabled", value="true")],
                                             delimiter="\n")
                              ], delimiter="\n"),
                   XmlElement(name="Features", element_name="Features",
                              content=[
                                  XmlElement(name="feature_core", element_name="Feature",
                                             attributes=[XmlAttribute(name="id", attribute="id", value="CORE")],
                                             delimiter="\n"),
                                  XmlElement(name="Feature_sdu", element_name="Feature",
                                             attributes=[XmlAttribute(name="id", attribute="id", value="SDU")],
                                             delimiter="\n")
                              ], delimiter="\n"),
                   XmlElement(name="intelligent_updating", element_name="intelligent_updating",
                              attributes=[
                                  XmlAttribute(name="Enabled", attribute="Enabled", value="false"),
                                  XmlAttribute(name="SubscriptionPolicy", attribute="SubscriptionPolicy",
                                               value="2DD71664-8D18-42C5-B3A0-FF0D289265BF")
                              ], delimiter="\n"),
                   XmlElement(name="customer", element_name="customer",
                              attributes=[
                                  XmlAttribute(name="id", attribute="id", value="ad936fd6-329d-e43b-cb15-3635be1058db")
                              ], delimiter="\n")

               ], delimiter="\n"
               )

alc_policy_fuzzed = Template(fields=[declaration_static, newline_static, alc_policy], name="alc_policy_fuzz")