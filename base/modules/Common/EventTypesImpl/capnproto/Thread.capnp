#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xa33e8a4febdc9325;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct ThreadEvent {
    enum EventType {
        start                    @0;
        end                      @1;
    }

    eventType                    @0 :EventType;
    sophosTID                    @1 :Common.SophosTID;
    sophosPID                    @2 :Common.SophosPID;
    parentSophosPID              @3 :Common.SophosPID;
    parentSophosTID              @4 :Common.SophosTID;
    endTime                      @5 :Common.Timestamp;
    startAddress                 @6 :UInt64;
    flags                        @7 :UInt32;
    imageName                    @8 :Text;
}