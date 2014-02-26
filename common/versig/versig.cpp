//versig.cpp
//
// Application to verify digitally-signed files and verify data files
// against Sophos manifest file.
//
// Usage:
//    versig -c<cert_file_path> [-r<crl_file_path>] -f<signed_file_path> [-d<data_dir_path>]
//
// Description:
//
//    versig -c<cert_file_path> -f<signed_file_path>
//       Verifies digital-signature on specified signed-file using specified
//       certificates-file.
//
//    versig -c<cert_file_path> -r<crl_file_path> -f<signed_file_path>
//       As above, but certificates-file is first checked against revocation list
//       in specified certificate-revocation-list-file.
//
//    versig -c<cert_file_path> -f<manifest_file_path> -d<data_dir_path>
//       Verifies digital-signature on specified Sophos manifest-file (manifest-
//       file is a signed-file) and confirms, for files specified in the manifest,
//       that corresponding files in the specified data-directory have same
//       checksum as recorded in the manifest.
//
//    versig -c<cert_file_path> -r<crl_file_path> -f<manifest_file_path> -d<data_dir_path>
//       As above, but certificates-file is first checked against revocation list
//       in specified certificate-revocation-list-file.
//
//    By default, versig runs silently, indicating success or failure of verification through
//    its exit value, one of the g_EXIT values listed below.
//
//    Output (useful for debugging) can be elicited by running versig with the optional
//    argument --silent-off. Note: this output is in English only - it is not intended for
//    customer use.

#include "manifest_file.h"
#include "verify_exceptions.h"
#include <sstream>

using namespace VerificationTool;
using namespace verify_exceptions;

const int g_EXIT_OK       = 0;
const int g_EXIT_BAD      = 1;
const int g_EXIT_BADARGS  = 2;
const int g_EXIT_BADCERT  = 3;
const int g_EXIT_BADCRYPT = 4;
const int g_EXIT_BADFILE  = 5;
const int g_EXIT_BADSIG   = 6;
const int g_EXIT_BADLOGIC = 7;


bool g_bSilent = true;	//Silent by default


static void Output
(
	string Msg	//[i] Message
)
//Output message if not in silent mode.
	//NB: Output in English. Use for debug only.
{
	if(!g_bSilent)
	{
		cout << Msg << endl;
	}
}

struct Arguments
{
    string SignedFilepath;	// Path to signed-file
    string CertsFilepath;	// Path to certificates-file
    string CRLFilepath;		// Path to CRL-file
    string DataDirpath;		// Path to directory containing data files
    bool fixDate;
    bool checkInstall;
    bool requireAllManifestFilesPresentOnDisk;
    bool requireAllDiskFilesPresentInManifest;
};


static bool ReadArgs
(
	int	argc,				//[i] Arguments count
	char*	argv[],				//[i] Argument values
	Arguments& args
)
//Read and process the command-line arguments
	//Return true if a valid set of arguments found
	//Else return false
{
	bool bGoodArgs = false;

	//Initialise
	args.SignedFilepath = "";
	args.CertsFilepath = "";
	args.CRLFilepath = "";
	args.DataDirpath = "";
	args.fixDate = true;
	args.checkInstall = false;
	args.requireAllManifestFilesPresentOnDisk = false;
	args.requireAllDiskFilesPresentInManifest = false;

	//Assign argument values
    for(int i=1; i<argc; i++)
    {
        string arg = argv[i];
        if( (arg.compare(0,2,"-c") == 0) && args.CertsFilepath.size() == 0 )
        {
            args.CertsFilepath = arg.substr(2);
        }
        else if( (arg.compare(0,2,"-d") == 0) && args.DataDirpath.size() == 0 )
        {
            args.DataDirpath = arg.substr(2);
        }
        else if( (arg.compare(0,2,"-f") == 0) && args.SignedFilepath.size() == 0 )
        {
            args.SignedFilepath = arg.substr(2);
        }
        else if( (arg.compare(0,2,"-r") == 0) && args.CRLFilepath.size() == 0 )
        {
            args.CRLFilepath = arg.substr(2);
        }
        else if( (arg.compare(0,12,"--silent-off") == 0) )
        {
            g_bSilent = false;
        }
        else if( (arg.compare(0,18,"--check-install-sh") == 0) )
        {
            args.checkInstall = true;
        }
        else if( (arg.compare(0,18,"--no-fix-date") == 0) )
        {
            args.fixDate = false;
        }
    }

    if( (args.SignedFilepath.size() > 0) && (args.CertsFilepath.size() > 0) )
    {
        bGoodArgs = true;
    }

	//Take action if bad arguments
	if(!bGoodArgs)
	{
		Output
		(
			string("Usage: ") + string(argv[0]) + string("\n") +
			string(" -c<certificate_file_path>\n") +
			string(" [-d<path to data files to be checked>]\n") +
			string(" -f<signed_file_path>\n") +
			string(" [-r<certificate_revocation_list_file_path>]\n") +
			string(" --silent-off\n") +
			string(" --check-install-sh\n")
		);
		return false;
	}

	//Arguments OK.
	return true;
}


int main
(
	int argc,		//[i] Count of arguments
	char* argv[]	//[i] Array of argument values
)
{
	Arguments args;

	//Read arguments
	if( !ReadArgs(argc,argv,args) )
	{
		return g_EXIT_BADARGS;
	}

	string SignedFilepath = args.SignedFilepath;	//Path to signed-file
	string CertsFilepath = args.CertsFilepath;	//Path to certificates-file
	string CRLFilepath = args.CRLFilepath;		//Path to CRL-file
	string DataDirpath = args.DataDirpath;		//Path to data-directory

	Output
	(
        string("Path to signed-file       = [") + SignedFilepath + string("]\n") +
        string("Path to certificates-file = [") + CertsFilepath + string("]\n") +
        string("Path to crl-file          = [") + CRLFilepath + string("]\n") +
        string("Path to data directory    = [") + DataDirpath + string("]\n")
	);

	//Process signed-file
	VerificationTool::ManifestFile MF;

	try
	{
		bool bOK = false;

		//Open the signed file (assumed to be manifest file)
		MF.Open(SignedFilepath, CertsFilepath, CRLFilepath, args.fixDate);

		//Validate signature
		bOK = MF.IsValid();
		if (!bOK)
		{
			// Shouldn't get here as signed_file::Open usually throws a helpful exception
			Output("unknown error in signed file\n");
			return g_EXIT_BAD;
		}

		//Validate data files against contents of manifest
		if(DataDirpath.size() > 0)
		{
			bOK = MF.DataCheck(DataDirpath);
			if( !bOK )
			{
				Output("unable to verify one or more data files\n");
				return g_EXIT_BADFILE;
			}
			else
			{
				Output("data files verified ok\n");
			}
		}

        if (args.checkInstall)
        {
            if (!MF.CheckFilePresent(".\\install.sh"))
            {
				Output("install.sh absent from Manifest\n");
				return g_EXIT_BADFILE;
            }
        }
	}

	catch ( ve_file& except )
	{
		ostringstream Msgstrm;
		Msgstrm << "Failed file:" << except << endl;
		Output(Msgstrm.str());
		return g_EXIT_BADFILE;
	}

	catch ( ve_crypt& except )
	{
		ostringstream Msgstrm;
		Msgstrm << "Failed cryptography:" << except << endl;
		Output(Msgstrm.str());
		return g_EXIT_BADCRYPT;
	}

	catch ( ve_missingsig& except )
	{
		ostringstream Msgstrm;
		Msgstrm << "Missing signature exception:" << except << endl;
		Output(Msgstrm.str());
		return g_EXIT_BADSIG;
	}

	catch ( ve_badsig& except )
	{
		ostringstream Msgstrm;
		Msgstrm << "Failed signature exception:" << except << endl;
		Output(Msgstrm.str());
		return g_EXIT_BADSIG;
	}

	catch ( ve_badcert& except )
	{
		ostringstream Msgstrm;
		Msgstrm << "Failed certificate exception:" << except << endl;
		Output(Msgstrm.str());
		return g_EXIT_BADCERT;
	}

	catch ( ve_logic& except )
	{
		ostringstream Msgstrm;
		Msgstrm << "Failed logic:" << except << endl;
		Output(Msgstrm.str());
		return g_EXIT_BADLOGIC;
	}

	catch (...)
	{
		Output("Failed: unexpected exception\n");
		return g_EXIT_BAD;
	}

	Output("File signed OK");
	return g_EXIT_OK;
}
