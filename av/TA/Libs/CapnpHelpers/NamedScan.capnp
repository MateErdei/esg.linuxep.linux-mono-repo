#------------------------------------------------------------------------------
# Copyright 2020 Sophos Limited. All rights reserved.
#
#------------------------------------------------------------------------------
@0x8bf3416dfec66ed9;

struct NamedScan {
    # The data required for a scan to run
    name                            @0  :Text;
    excludePaths                    @1  :List(Text);
    sophosExtensionExclusions       @2  :List(Text);
    userDefinedExtensionInclusions  @3  :List(Text);
    scanArchives                    @4  :Bool;
    scanAllFiles                    @5  :Bool;
    scanFilesWithNoExtensions       @6  :Bool;
    scanHardDrives                  @7  :Bool;
    scanCDDVDDrives                 @8  :Bool;
    scanNetworkDrives               @9  :Bool;
    scanRemovableDrives             @10 :Bool;
}
