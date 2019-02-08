#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
#
# Built-in Types https:#capnproto.org/language.html#built-in-types
#
# Void:               Types
# Boolean:            Bool
# Integers:           Int8, Int16, Int32, Int64
# Unsigned Integers:  UInt8, UInt16, UInt32, UInt64
# Floating-point:     Float32, Float64
# Blobs:              Text, Data
# Lists:              List(T)
#
# struct <struct-name> {
#     <field-name-0> @0 :<type>; # must start @0
#     ...
#     <field-name-n> @n-1 :<type>; # n-1th field
#     ...
# }
# 
# <union-name> :union {
#   <field-name-0> @0 :<type>; # must start @0
#   ...
#   <field-name-1> @n-1 :<type>; # n-1th field
#   ...
# }
#
@0xc95e525a140fd031;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

# NOTE: 'Data' can be cast right back to a struct* type
using EventID = UInt32;
using FileAttributes = UInt32;
using GUID = Data;
using PID = UInt64;
using SID = Data;
using DACL = Data;
using SHA1 = Data;
using SHA256 = Data;
using TID = UInt64;
using Timestamp = UInt64;
using IpAddr = Data; # In network byte-ordering

struct FileId128 {
    sLowPart             @0 :UInt64;
    sHighPart            @1 :UInt64;
}

struct FileId {
    fileId               @0 :FileId128;
    volumeGUID           @1 :GUID;
}

struct FileInfo {
    numberOfHardLinks    @0 :UInt32;
    fileAttributes       @1 :FileAttributes;
}

struct FileTimestamps {
    creationTime         @0 :Timestamp;
    lastAccessTime       @1 :Timestamp;
    lastWriteTime        @2 :Timestamp;
    changeTime           @3 :Timestamp;
}

struct FileType {
    fileTypeClass        @0 :UInt32;
    fileType             @1 :UInt32;
}

struct SocketAddr {
    # Inet type can be inferred from the address size
    address                     @0 :IpAddr; # In network byte-ordering
    port                        @1 :UInt16;
}

struct IpFlow {
    sourceAddress       @0 :SocketAddr;
    destinationAddress  @1 :SocketAddr;
    protocol            @2 :UInt8;
}

struct Pathname {
    flags                @0 :UInt16;
    fileSystemType       @1 :UInt32;
    driveLetter          @2 :UInt8;
    pathname             @3 :Text;
    openName             @4 :TextOffsetLength;
    volumeName           @5 :TextOffsetLength;
    shareName            @6 :TextOffsetLength;
    extensionName        @7 :TextOffsetLength;
    streamName           @8 :TextOffsetLength;
    finalComponentName   @9 :TextOffsetLength;
    parentDirName       @10 :TextOffsetLength;
}

# pathname
#
# This is the pathname used to open the file.  The pathname data has
# several attributes:
#
#    pathname.flags
#
#    This is a 16bit integer with various flags for the pathname.  The values
#    are the following and are defined in SgEventJournalFormat.hpp:
#
#    SG_EVT_JRN_PATHNAME_NORMALIZED          0x0001  This is the normalized pathname that the kernel was able to determine.  If
#                                                      this flag is not set, then the pathname is the one used in the open call.
#    SG_EVT_JRN_PATHNAME_NETWORK             0x0002  If set, the pathname is a network device
#    SG_EVT_JRN_PATHNAME_IS_SNAPSHOT         0x0004  If set, the pathname is a Windows snapshot volume.
#    SG_EVT_JRN_PATHNAME_DRV_LETTER_SET      0x0008  If set, the drive letter character is set
#    SG_EVT_JRN_PATHNAME_HAS_MULTIBYTE_UTF8  0x0010  If set, the pathname contains one or more multi-byte UTF8 characters
#    SG_EVT_JRN_PATHNAME_OFFSETS_SET         0x0020  If set, the pathname component lengths and offsets are set.  Some may still be
#                                                      set to zero if not used (i.e. shareName and streamName)
#    SG_EVT_JRN_PATHNAME_MAY_BE_REPLACED     0x0040  If set, this flag indicates that the file referenced by the pathname may
#                                                      have been replaced during this call.  Examples include opening a file
#                                                      with SUPERCEDE, renaming a file with FILE_RENAME_REPLACE_IF_EXISTS, etc...
#
#    pathname.fileSystemType
#
#    This value indicates the filesystem type using Microsoft defined values and
#    are defined in SgEventJournalFormat.hpp:
#
#    SG_MS_FILESYSTEM_TYPE_UNKNOWN       0     an unknown file system type
#    SG_MS_FILESYSTEM_TYPE_RAW           1     Microsoft's RAW file system                  (\FileSystem\RAW)
#    SG_MS_FILESYSTEM_TYPE_NTFS          2     Microsoft's NTFS file system                 (\FileSystem\Ntfs)
#    SG_MS_FILESYSTEM_TYPE_FAT           3     Microsoft's FAT file system                  (\FileSystem\Fastfat)
#
#    pathname.driveLetter
#
#    This is the drive letter (if set).  A value of 67 is the 'C' drive.
#
#    pathname.pathname
#
#    This is the pathname in UTF8.  The Windows kernel supports pathnames of
#    up to 32767 UTF16 characters.  When converted to UTF8, the pathname could
#    take up to 98301 characters since a single UTF16 character could be
#    converted into a 3 byte UTF8 multi-byte character.
#
# The following fields represent full or partial views into the pathname.
# Each consists of an offset into the pathname in bytes and the length
# in bytes.  For example, we will use the following pathname for illustration:
#
#       \Device\HarddiskVolume1\Documents and Settings\MyUser\My Documents\Test Results.txt:stream1
#
#    pathname.openName                Full pathname the stream was opened with (in bytes)
#                                     The name offset must be 0 since it is the complete name
#
#    pathname.volumeName              Volume name = "\Device\HarddiskVolume1"
#                                     The volume offset must be 0 since it starts the name
#
#    pathname.shareName               The share component of the file name requested.
#                                     The offset and length will always be 0 for local files.
#
#    pathname.extensionName           Extension = "txt"
#
#    pathname.streamName              Stream = ":stream1"
#
#    pathname.finalComponentName      FinalComponent = "Test Results.txt:stream1"
#                                     This may not be set if the opened pathname is a top level
#                                       directory for a volume (i.e. "\Device\HarddiskVolume1\"
#                                       In this example, the ParentDirName would be "\")
#
#    pathname.parentDirName           ParentDir = "\Documents and Settings\MyUser\My Documents\"
#
#   Here would be a similar breakdown in Linux for:
#
#               /home/johndoe/Documents/Sample.odt
#
#   pathname.openName:              /home/johndoe/Documents/Sample.odt
#   pathname.finalComponentName:    Sample.odt
#   pathname.parentDirName:         /home/johndoe/Documents/
#   pathname.extensionName:         odt
#   pathname.streamName:            is not used since this represents alternate data streams for files
#                                   on NTFS file systems.
#   pathname.volumeName:            is not used since Linux the file system is not part of the pathname.
#                                   For example, the /dev/sda1 file system may be mounted at /
#   pathname.shareName:             is also probably not used since the remote share is not part of the local pathname.

struct OptionalTimestamp { # Use this when when a Timestamp is immediately enclosed in a "optional :group" at the top level.
    value                @0 :Timestamp;
}

struct OptionalUInt8 { # Use this when when a UInt8 is immediately enclosed in a "optional :group" at the top level.
    value                @0 :UInt8;
}

struct OptionalUInt16 { # Use this when when a UInt16 is immediately enclosed in a "optional :group" at the top level.
    value                @0 :UInt16;
}

struct OptionalUInt32 { # Use this when when a UInt32 is immediately enclosed in a "optional :group" at the top level.
    value                @0 :UInt32;
}

struct OptionalUInt64 { # Use this when when a UInt64 is immediately enclosed in a "optional :group" at the top level.
    value                @0 :UInt64;
}

struct SophosPID { # Sophos Process ID
    osPID                @0 :PID; # Actual OS Process ID
    createTime           @1 :Timestamp;
}

struct SophosTID  { # Sophos Thread ID
    osTID                @0 :TID;     # Actual OS Thread ID
    createTime           @1 :Timestamp;
}

struct RedirectionInformation {

    # Redirection state flags. In theory both can be set
    redirectionState            @0 :UInt8;

    # Only used if redirectionState contains RedirectionStateInProgress
    originalDestination         @1 :SocketAddr;
    targetSophosPID             @2 :SophosPID; # May not be available

    # Only used if redirectionState contains RedirectionStateRedirected
    originalProcessPath         @3 :Pathname;
}

struct TextOffsetLength {
    length               @0 :UInt32; # Length in bytes
    offset               @1 :UInt32; # Offset in bytes
}

struct VersionInfo3 {
    major                @0 :UInt32;
    minor                @1 :UInt32;
    maintenance          @2 :UInt32;
}

struct VersionInfo4 {
    major                @0 :UInt32;
    minor                @1 :UInt32;
    maintenance          @2 :UInt32;
    build                @3 :UInt32;
}

struct UserSID {
    username             @0 :Text;
    sid                  @1 :SID; # Windows only
    domain               @2 :Text;
}

struct NetworkAddress {
    address             @0 :Text;
}
