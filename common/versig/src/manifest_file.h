// verify.h: interface for the verify facade class.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#if !defined(_MANIFEST_FILE_H_INCLUDED_)
#define _MANIFEST_FILE_H_INCLUDED_

#include "signed_file.h"
#include "digest_body.h"

namespace VerificationTool {

class ManifestFile
	:public SignedFile
{
public:
	typedef digest_file_body::files_iter files_iter;

	ManifestFile();
	~ManifestFile();

	bool ReadBody();
	//Read body of manifest file

	bool DataCheck
	(
		string DataDirpath	//[i] Path to directory containing data files.
	);
	//Confirm files in data-directory match contents of manifest.

    /**
     * Verify that relFilePath is included in this manifest file
     */
    bool CheckFilePresent(string relFilePath);

private:
	digest_file_body m_DigestBody;

	ManifestFile::files_iter FileRecordsBegin();
	//Return iterator identifying first file-record in ManifestFile

	ManifestFile::files_iter FileRecordsEnd();
	//Return iterator identifying last file-record in ManifestFile

	void RequireValid();
	//Throw XXX exception if ManifestFile status IS NOT valid
};

} // namespace VerificationTool


#endif // !defined(_MANIFEST_FILE_H_INCLUDED_)
