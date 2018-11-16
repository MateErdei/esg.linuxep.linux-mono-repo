#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0x9710073a0f7a612c;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct ProcessPropertyEvent {
    enum EventType {
        set                      @0;
        unset                    @1;
    }

    eventType                    @0 :EventType;
    sophosPID                    @1 :Common.SophosPID;
    sophosTID                    @2 :Common.SophosTID;
    propertyDataLength           @3 :UInt32;
    propertyId                   @4 :UInt32;
    createTime                   @5 :Common.Timestamp;
    targetSPID                   @6 :Common.SophosPID;
    propertyData                 @7 :Data;
}