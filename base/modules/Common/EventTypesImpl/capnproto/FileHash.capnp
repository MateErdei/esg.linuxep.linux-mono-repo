#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xc72e4f2652481a3e;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct FileHashEvent {
    enum EventType {
        set                      @0;
        unset                    @1;
    }

    eventType                    @0 :EventType;
    fileId                       @1 :Common.FileId;
    sha256                       @2 :Common.SHA256;
    fileSize                     @3 :UInt64;
}
