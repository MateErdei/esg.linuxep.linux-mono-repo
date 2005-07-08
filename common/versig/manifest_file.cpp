// manifest_file.cpp: interface for the Manifest facade class.
//
//  20050322 Modified for SAV for Linux from verify.cpp.
//
//////////////////////////////////////////////////////////////////////

#include "manifest_file.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <assert.h>
#include "verify_exceptions.h"

using namespace verify_exceptions;

namespace VerificationTool {

ManifestFile::ManifestFile()
{
}


ManifestFile::~ManifestFile()
{
}


bool ManifestFile::ReadBody()
//Read body of manifest file
	//Returns true if format OK.
	//Otherwise returns false.
{
	istringstream FileBody(m_DigestBuffer.file_body());
	FileBody >> m_DigestBody;
	if (!FileBody) 
   	{ 
		m_Status = bad_syntax; 
		return false;
   	}

	//All ok
	return true;
}



bool ManifestFile::DataCheck
(
	string DataDirpath	//[i] Path to directory containing data files.
)
//Confirm files in data-directory match contents of manifest.
	//Returns true if checksums of all actual files match those recorded in 
	//the manifest. Otherwise, returns false.
	//Only checks files recorded in the manifest. For SAV for Linux, the set
	//of file in a CID may be a subset of the files recorded in the manifest,
	//so missing files are treated as OK.		
{
	if (DataDirpath.size() == 0)
	{
		//Missing files are not a problem
		return true;
	}
	
	bool bAllFilesOK = true;
	ManifestFile::files_iter p = FileRecordsBegin();
	while( bAllFilesOK && (p != FileRecordsEnd()) )
	{
		string Msg = "";
		switch (p->verify_file(DataDirpath)) 
		{
			case file_info::file_ok:
			case file_info::file_missing:
				break;

			case file_info::file_invalid:
			default:
				bAllFilesOK = false;
				break;
		}
		p++;
	}

	return bAllFilesOK;
}


void ManifestFile::RequireValid()
//Throw ve_logic exception if ManifestFile status IS NOT valid
{
	if(m_Status != valid)
	{
		throw ve_logic(m_Status);
	}
}

ManifestFile::files_iter ManifestFile::FileRecordsBegin() 
//Return iterator identifying first file-record in ManifestFile
{
	RequireValid();
	return m_DigestBody.files_begin();
}

ManifestFile::files_iter ManifestFile::FileRecordsEnd()
//Return iterator identifying last file-record in ManifestFile 
{
	RequireValid();
	return m_DigestBody.files_end();
}

} // namespace VerificationTool
