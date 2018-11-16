#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xbe7c668df96d5f4c;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct DriverEvent {
    enum EventType {
        loaded                   @0;
        unloaded                 @1;
    }

    eventType                    @0 :EventType;
    version                      @1 :Common.VersionInfo4;
    osVersion                    @2 :Common.VersionInfo3;
    flags                        @3 :UInt32;
}
