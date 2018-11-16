#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0x9d67f7dbb8be2944;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct SystemEvent {
    enum EventType {
         shutdown                @0;
         timeChange              @1;
    }

    eventType                    @0 :EventType;
    osVersion                    @1 :Common.VersionInfo3;
    flags                        @2 :UInt32;
}