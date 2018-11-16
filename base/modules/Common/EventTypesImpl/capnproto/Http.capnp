#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xb4cff664a9c15b39;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct HttpEvent {
    sophosPID                    @0 :Common.SophosPID;
    sophosTID                    @1 :Common.SophosTID;
    ipFlow                       @2 :Common.IpFlow;
    redirectionInformation       @3 :Common.RedirectionInformation;
    url                          @4 :Text;
    headers                      @5 :Text;
}
