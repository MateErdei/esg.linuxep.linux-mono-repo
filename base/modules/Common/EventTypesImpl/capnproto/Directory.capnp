#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xbb9227a822355395;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct DirectoryEvent {
    enum EventType {
        created                  @0;
        renamed                  @1;
        deleted                  @2;
        permissionsModified      @3;
        ownershipModified        @4;
    }

    eventType                    @0 :EventType;
    sophosPID                    @1 :Common.SophosPID;
    sophosTID                    @2 :Common.SophosTID;
    pathname                     @3 :Common.Pathname;
    size                         @4 :Common.OptionalUInt64;
    targetPathname               @5 :Common.Pathname;
    fileId                       @6 :Common.FileId;
    timestamps                   @7 :Common.FileTimestamps;
    fileInfo                     @8 :Common.FileInfo;
    fileType                     @9 :Common.FileType;
    dacl                        @10 :Common.DACL;
    owner                       @11 :Common.SID;
    newTimestamps               @12 :Common.FileTimestamps;
}

#------------------------------------------------------------------------------
# sophosPID
#
# This field contains the Sophos PID of the process that opened the handle to
# the directory.
#
# sophosTID
#
# This field contains the Sophos TID of the thread that opened the handle to
# the directory.
#
# fileSize
#
# This is the size of the directory in bytes.
#
# pathname
#
# This is the pathname used to open the directory.  The pathname data has
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
# targetPathname
#
# This is the target pathname used in the event.  It should only be set for
# rename and hard link events.
#
# fileId
#
# This is the file ID and volume GUID of the directory.  This will only be set
# for local directories on NTFS and ReFS file systems.
#
# timestamps
#
# These are the various file timestamps for the directory before the event occurred.
#
# fileInfo
#
# This field contains the number of hardlinks to the directory and the directory attributes.
# These values are from before the event, except for create hardlink events where the
# number of hard links has been incremented by one to signify the new hard link.
#
# fileType
#
# This field defines the type of file (binary, data, or other) and the specifi
# type if appropriate.  The values are the following and are defined in SgEventJournalFormat.hpp:
#
#    fileType.fileTypeClass:
#
#    SG_EVT_JRN_FILE_TYPE_CLASS_UNKNOWN      0
#    SG_EVT_JRN_FILE_TYPE_CLASS_DIRECTORY    1
#    SG_EVT_JRN_FILE_TYPE_CLASS_BINARY       2
#    SG_EVT_JRN_FILE_TYPE_CLASS_DATA         3
#    SG_EVT_JRN_FILE_TYPE_CLASS_OTHER        4
#
#    If we were able to read in the start of the file and analyze it, we will
#    determine if it is a WinPE or ELF binary regardless of the extension.
#    If the file is not a binary (or is zero bytes in length), we will use
#    the following extensions to classify the file:
#
#      Binary:  exe, dll, sys
#      Data:  doc, docx, xls, xlsx, ppt, pptx, pdf, rtf, wpd
#
#    fileType.fileType:
#
#    SG_EVT_JRN_FILE_TYPE_UNKNOWN            0
#    SG_EVT_JRN_FILE_TYPE_SED_WINPE          1
#    SG_EVT_JRN_FILE_TYPE_ELF_BINARY         2
#
# dacl
#
# This is the DACL set on the directory in the permissions changed event.
#
# owner
#
# This is the SID set on the directory for the owner changed event.
#
# newTimestamps
#
# These are the new timestamps set on the directory for the timestamps changed event.  Description of
# the values can be found at:
#
#    https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/ns-wdm-_file_basic_information
#
#    If you specify a value of zero for any of the XxxTime members of the FILE_BASIC_INFORMATION structure,
#    the ZwSetInformationFile function keeps a file's current setting for that time.
#
#    The file system updates the values of the LastAccessTime, LastWriteTime, and ChangeTime members as
#    appropriate after an I/O operation is performed on a file. However, a driver or application can request
#    that the file system not update one or more of these members for I/O operations that are performed on the
#    caller's file handle by setting the appropriate members to -1. The caller can set one, all, or any other
#    combination of these three members to -1. Only the members that are set to -1 will be unaffected by I/O
#    operations on the file handle; the other members will be updated as appropriate.
#
#------------------------------------------------------------------------------
