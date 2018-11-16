#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xc4109a4648b7a426;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct DnsEvent {
    enum EventType {
        request                  @0;
    }

    eventType                    @0 :EventType;
    sophosPID                    @1 :Common.SophosPID;
    sophosTID                    @2 :Common.SophosTID;
    name                         @3 :Text;
}
