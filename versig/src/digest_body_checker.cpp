 // digest_body_checker.cpp: implementation of the digest_body_checker class.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#include "digest_body.h"
#include "crypto_utils.h"
#include "print.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <iostream>


namespace VerificationTool {

file_info::verify_result file_info::verify_file(const string& root_path) const
{
	//Obtain (relative) path to file as recorded in manifest file
		//Allow for preceeding '.'
	string rel_path = path();
	if( rel_path[0] == '.' )
	{
		rel_path = rel_path.substr(1);
	}

	//Calculate the actual path to file
		//Convert any '\' to '/' because we are on Unix

	string file_path = root_path;
	if (rel_path[0] != '\\')
	{
		file_path += '/';
	}
	file_path += rel_path;

	replace(file_path.begin(),file_path.end(),'\\','/');

	//Open file
	fstream file(file_path.c_str(), ios::in | ios::binary);
	if (!file) // could not open file
	{
		return file_missing;
	}

	//Compare recorded and actual checksum
	if (checksum().size() == VerificationToolCrypto::sha1size())
	{
	    std::string actualSHA1=VerificationToolCrypto::sha1sum(file);
		if (checksum() != actualSHA1)
		{
//		    PRINT("Incorrect SHA1: expected="<<checksum()<< " actual="<<actualSHA1);
			return file_invalid;
		}
	}
	else if (checksum().size() == VerificationToolCrypto::sha512size())
	{
		if (checksum() != VerificationToolCrypto::sha512sum(file))
		{
			return file_invalid;
		}
	}
	else
	{
		return file_invalid;
	}

	// Check SHA256 comment
	if (!sha256().empty())
	{
        file.clear();
		file.seekg(0, file.beg);
		std::string actualSHA256 = VerificationToolCrypto::sha256sum(file);
//        PRINT("Checking SHA256: expected="<<sha256()<< " actual="<<actualSHA256);
		if (sha256() != actualSHA256)
		{
			return file_invalid;
		}
	}


	//File exists and has recorded checksum
	return file_ok;
}

} // namespace VerificationTool
