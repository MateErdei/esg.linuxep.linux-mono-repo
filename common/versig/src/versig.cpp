// versig.cpp
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

#include "SophosCppStandard.h"

#include "Arguments.h"
#include "manifest_file.h"
#include "verify_exceptions.h"

#include <cassert>
#include <sstream>
#include <tuple>

using namespace VerificationTool;
using namespace verify_exceptions;

const int g_EXIT_OK = 0;
const int g_EXIT_BAD = 1;
const int g_EXIT_BADARGS = 2;
const int g_EXIT_BADCERT = 3;
const int g_EXIT_BADCRYPT = 4;
const int g_EXIT_BADFILE = 5;
const int g_EXIT_BADSIG = 6;
const int g_EXIT_BADLOGIC = 7;

static bool g_bSilent = true; // Silent by default

static void Output(const string& Msg //[i] Message
)
// Output message if not in silent mode.
// NB: Output in English. Use for debug only.
{
    if (!g_bSilent)
    {
        cout << Msg << endl;
    }
}

static bool ReadArgs(const std::vector<std::string>& argv, Arguments& args)
{
    assert(!argv.empty());
    // Read and process the command-line arguments
    // Return true if a valid set of arguments found
    // Else return false
    g_bSilent = true;
    bool bGoodArgs = false;

    // Initialise
    // Assign argument values
    for (const auto& arg : argv)
    {
        if ((arg.compare(0, 2, "-c") == 0) && args.CertsFilepath.empty())
        {
            args.CertsFilepath = arg.substr(2);
        }
        else if ((arg.compare(0, 2, "-d") == 0) && args.DataDirpath.empty())
        {
            args.DataDirpath = arg.substr(2);
        }
        else if ((arg.compare(0, 2, "-f") == 0) && args.SignedFilepath.empty())
        {
            args.SignedFilepath = arg.substr(2);
        }
        else if ((arg.compare(0, 2, "-r") == 0) && args.CRLFilepath.empty())
        {
            args.CRLFilepath = arg.substr(2);
        }
        else if ((arg.compare(0, 12, "--silent-off") == 0))
        {
            g_bSilent = false;
        }
        else if ((arg.compare(0, 18, "--check-install-sh") == 0))
        {
            args.checkInstall = true;
        }
        else if ((arg.compare(0, 18, "--no-fix-date") == 0))
        {
            args.fixDate = false;
        }
        else if (arg == "--require-sha256")
        {
            args.requireSHA256 = true;
        }
        else if (arg == "--no-require-sha256")
        {
            args.requireSHA256 = false;
        }
        else if (arg == "--allow-sha1-signature")
        {
            args.allowSHA1signature = true;
        }
        else if (arg == "--deny-sha1-signature")
        {
            args.allowSHA1signature = false;
        }
    }

    if (!args.SignedFilepath.empty() && !args.CertsFilepath.empty())
    {
        bGoodArgs = true;
    }

    // Take action if bad arguments
    if (!bGoodArgs)
    {
        g_bSilent = false;
        Output(
            string("Usage: ") + string(argv[0]) + string("\n") + string(" -c<certificate_file_path>\n") +
            string(" [-d<path to data files to be checked>]\n") + string(" -f<signed_file_path>\n") +
            string(" [-r<certificate_revocation_list_file_path>]\n") + string(" --silent-off\n") +
            string(" --check-install-sh\n"));
        return false;
    }

    // Arguments OK.
    return true;
}

static int versig_operation(const Arguments& args)
{
    string SignedFilepath = args.SignedFilepath; // Path to signed-file
    string CertsFilepath = args.CertsFilepath;   // Path to certificates-file
    string CRLFilepath = args.CRLFilepath;       // Path to CRL-file
    string DataDirpath = args.DataDirpath;       // Path to data-directory

    Output(
        string("Path to signed-file       = [") + SignedFilepath + string("]\n") +
        string("Path to certificates-file = [") + CertsFilepath + string("]\n") +
        string("Path to crl-file          = [") + CRLFilepath + string("]\n") +
        string("Path to data directory    = [") + DataDirpath + string("]\n"));

    // Process signed-file
    VerificationTool::ManifestFile MF;

    try
    {
        // Open the signed file (assumed to be manifest file)
        MF.Open(args);

        // Validate signature
        bool bOK = MF.IsValid();
        if (!bOK)
        {
            // Shouldn't get here as signed_file::Open usually throws a helpful exception
            Output("unknown error in signed file\n");
            return g_EXIT_BAD;
        }

        // Validate data files against contents of manifest
        if (!DataDirpath.empty())
        {
            std::tuple<bool, std::string> dataCheckResult = MF.DataCheck(DataDirpath, args.requireSHA256);
            bOK = std::get<0>(dataCheckResult);

            if (!bOK)
            {
                Output(std::get<1>(dataCheckResult) + "\n");
                return g_EXIT_BADFILE;
            }
            else
            {
                Output("data files verified ok\n");
            }
        }

        if (args.checkInstall)
        {
            if (!(MF.CheckFilePresent(".\\install.sh") || MF.CheckFilePresent("install.sh") || MF.CheckFilePresent("./install.sh")))
            {
                Output("install.sh absent from Manifest\n");
                return g_EXIT_BADFILE;
            }
        }
    }

    catch (ve_file& except)
    {
        ostringstream Msgstrm;
        Msgstrm << "Failed file:" << except << endl;
        Output(Msgstrm.str());
        return g_EXIT_BADFILE;
    }

    catch (ve_crypt& except)
    {
        ostringstream Msgstrm;
        Msgstrm << "Failed cryptography:" << except << endl;
        Output(Msgstrm.str());
        return g_EXIT_BADCRYPT;
    }

    catch (ve_missingsig& except)
    {
        ostringstream Msgstrm;
        Msgstrm << "Missing signature exception:" << except << endl;
        Output(Msgstrm.str());
        return g_EXIT_BADSIG;
    }

    catch (ve_badsig& except)
    {
        ostringstream Msgstrm;
        Msgstrm << "Failed signature exception:" << except << endl;
        Output(Msgstrm.str());
        return g_EXIT_BADSIG;
    }

    catch (ve_badcert& except)
    {
        ostringstream Msgstrm;
        Msgstrm << "Failed certificate exception:" << except << endl;
        Output(Msgstrm.str());
        return g_EXIT_BADCERT;
    }

    // catch ( ve_logic& except )
    //{
    //	ostringstream Msgstrm;
    //	Msgstrm << "Failed logic:" << except << endl;
    //	Output(Msgstrm.str());
    //	return g_EXIT_BADLOGIC;
    //}
    catch (const std::bad_alloc& except)
    {
        ostringstream Msgstrm;
        Msgstrm << "Failed: std::bad_alloc" << endl;
        Output(Msgstrm.str());
        return g_EXIT_BAD;
    }
    catch (const std::runtime_error& ex)
    {
        Output(ex.what());
        return g_EXIT_BAD;
    }
    catch (const std::exception& except)
    {
        ostringstream Msgstrm;
        Msgstrm << "Failed: std::exception:" << except.what() << endl;
        Output(Msgstrm.str());
        return g_EXIT_BAD;
    }

    catch (...)
    {
        Output("Failed: unexpected exception\n");
        return g_EXIT_BAD;
    }

    Output("File signed OK");
    return g_EXIT_OK;
}

int versig_main(const std::vector<std::string>& argv)
{
    Arguments args;

    // Read arguments
    if (!ReadArgs(argv, args))
    {
        return g_EXIT_BADARGS;
    }
    return versig_operation(args);
}

int versig_main(
    int argc,    //[i] Count of arguments
    char* argv[] //[i] Array of argument values
)
{
    std::vector<std::string> argvv;

    assert(argc >= 1);
    for (int i = 0; i < argc; i++)
    {
#if 03 == CPPSTD
        argvv.push_back(std::string(argv[i]));
#else
        argvv.emplace_back(argv[i]);
#endif
    }

    assert(!argvv.empty());
    return versig_main(argvv);
}
