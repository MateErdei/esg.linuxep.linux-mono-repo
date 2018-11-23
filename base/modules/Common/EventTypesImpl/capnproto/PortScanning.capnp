#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xda2cf101a516f0a5;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct PortEvent {
    enum EventType {
        opened                  @0;
        closed                  @1;
        connected               @2;
        scanned                 @3;
    }

    eventType                   @0 :EventType;
    connection                  @1 :Common.IpFlow;
}