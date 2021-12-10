#------------------------------------------------------------------------------
# Copyright 2020 Sophos Limited. All rights reserved.
#
#------------------------------------------------------------------------------
@0xc8e49ccf46ec1bca;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

struct NamedScan {
    # The data required for a scan to run
    name                            @0  :Text;
    excludePaths                    @1  :List(Text);
    sophosExtensionExclusions       @2  :List(Text);
    userDefinedExtensionInclusions  @3  :List(Text);
    scanArchives                    @4  :Bool;
    scanImages                      @5  :Bool;
    scanAllFiles                    @6  :Bool;
    scanFilesWithNoExtensions       @7  :Bool;
    scanHardDrives                  @8  :Bool;
    scanCDDVDDrives                 @9  :Bool;
    scanNetworkDrives               @10  :Bool;
    scanRemovableDrives             @11 :Bool;
}
