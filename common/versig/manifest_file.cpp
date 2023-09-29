// manifest_file.cpp: interface for the Manifest facade class.
//
//  20050322 Modified for SAV for Linux from verify.cpp.
//
//////////////////////////////////////////////////////////////////////

#include "manifest_file.h"

#include "verify_exceptions.h"

#include <cassert>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace verify_exceptions;

namespace VerificationTool
{
    ManifestFile::ManifestFile()
    {
    }

    bool ManifestFile::ReadBody()
    // Read body of manifest file
    // Returns true if format OK.
    // Otherwise returns false.
    {
        istringstream FileBody(m_DigestBuffer.file_body());
        FileBody >> m_DigestBody;

        // All ok, allow empty manifests.
        return true;
    }

    std::tuple<bool, std::string> ManifestFile::DataCheck(
        string DataDirpath, //[i] Path to directory containing data files.
        bool requireSHA256)
    // Confirm files in data-directory match contents of manifest.
    // Returns a tuple value of true and an empty error string if checksums of all actual files match those recorded in
    // the manifest. Otherwise, returns tuple value of false and provides an errorMessage string for reporting later.
    // Only checks files recorded in the manifest. For SAV for Linux, the set
    // of file in a CID may be a subset of the files recorded in the manifest,
    // so missing files are treated as OK.
    {
        if (DataDirpath.empty())
        {
            // Missing files are not a problem
            return std::make_tuple(true, "");
        }

        bool bAllFilesOK = true;
        std::string errorMessage("");
        ManifestFile::files_iter p = FileRecordsBegin();
        while (bAllFilesOK && (p != FileRecordsEnd()))
        {
            if (requireSHA256 && p->sha256().empty())
            {
                bAllFilesOK = false;
            }
            switch (p->verify_file(DataDirpath))
            {
                case file_info::file_ok:
                case file_info::file_missing:
                    break;

                case file_info::file_invalid:
                    errorMessage = "Invalid file found: " + p->path();
                    bAllFilesOK = false;
                    break;
                default:
                    bAllFilesOK = false;
                    break;
            }
            ++p;
        }

        return std::make_tuple(bAllFilesOK, errorMessage);
    }

    /**
     * Verify that relFilePath is included in this manifest file
     */
    bool ManifestFile::CheckFilePresent(string relFilePath)
    {
        ManifestFile::files_iter p = FileRecordsBegin();
        while (p != FileRecordsEnd())
        {
            if (p->path() == relFilePath)
            {
                return true;
            }
            ++p;
        }
        return false;
    }

    // void ManifestFile::RequireValid()
    ////Throw ve_logic exception if ManifestFile status IS NOT valid
    //{
    //	if(m_Status != valid)
    //	{
    //		throw ve_logic(m_Status);
    //	}
    //}

    ManifestFile::files_iter ManifestFile::FileRecordsBegin()
    // Return iterator identifying first file-record in ManifestFile
    {
        // RequireValid();
        return m_DigestBody.files_begin();
    }

    ManifestFile::files_iter ManifestFile::FileRecordsEnd()
    // Return iterator identifying last file-record in ManifestFile
    {
        // RequireValid();
        return m_DigestBody.files_end();
    }

} // namespace VerificationTool
