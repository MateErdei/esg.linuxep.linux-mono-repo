///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_VERSIONEDCOPY_H
#define EVEREST_BASE_VERSIONEDCOPY_H

#include <Common/FileSystem/IFileSystem.h>

namespace Installer
{
    namespace VersionedCopy
    {
        /**
         * Implementation of VersionedCopy:
         *
         * for a distributed file: files/base/lib64/libfoobar.so.5.6.3
         *
         * 1) Define an installation path $INST/base/lib64/libfoobar.so.5.6.3
         * 2) Create directories if they don't exist
         * 3) Compare dist vs. inst files, and exit if they are the same
         * 4) Copy the changed file to the next available extension in the sequence <file>.0 <file>.1
         *    a) Find the current digit by reading the install file symlink.
         * 5) Symlink the installation path to the new file
         * 6) Create symlinks for libraries.
         *
         * e.g.
         * $INST/base/lib64/libfoobar.so.5.6.3.1
         * $INST/base/lib64/libfoobar.so.5.6.3 -> libfoobar.so.5.6.3.1
         * $INST/base/lib64/libfoobar.so.5.6 -> libfoobar.so.5.6.3
         * $INST/base/lib64/libfoobar.so.5 -> libfoobar.so.5.6
         * $INST/base/lib64/libfoobar.so -> libfoobar.so.5
         */
        class VersionedCopy
        {
        public:
            /**
             * Get the digit block from the end of a string.
             * @param s String of the form <anything>.<number>
             * @return int <number>
             *
             * NB: Exposed for unit testing.
             */
            static int getDigitFromEnd(const std::string& s);

            /**
             * Convert args and environment into filename, DIST, INST for versionedCopy()
             * @param argc from main
             * @param argv from main
             * @return exit code
             */
            static int versionedCopyMain(int argc, char* argv[]);

            /**
             * Perform versioned copy as above.
             * Exposed for unit testing.
             *
             * @param filename
             * @param DIST
             * @param INST
             * @return process exit code
             */
            static int versionedCopy(const Path& filename, const Path& DIST, const Path& INST);
        private:
            /**
             * Determine if two files are the same.
             * @param file1
             * @param file2
             * @return true if the files are the same (or both don't exist)
             */
            static bool same(const Path& file1, const Path& file2);
        };
    }
}

#endif //EVEREST_BASE_VERSIONEDCOPY_H
