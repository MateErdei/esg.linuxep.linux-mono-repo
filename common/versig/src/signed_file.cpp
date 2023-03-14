// signed_file.cpp: interface for the SignedFile facade class.
//
//  20050322 Modified for SAV for Linux from verify.cpp
//
//////////////////////////////////////////////////////////////////////

#include "signed_file.h"

#include "verify_exceptions.h"

#include <cassert>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace verify_exceptions;

namespace VerificationTool
{
    SignedFile::SignedFile() : m_Status(uninitialised) //, m_DigestBufferMaxSizeBytes(0)
    {
    }

    SignedFile::~SignedFile() {}

    // void SignedFile::SetDigestSizeLimit
    //(
    //	unsigned long MaxSizeBytes
    //)
    ////Sets the maximum digest buffer size
    //	//Call this before open if you really need to
    //{
    //	m_DigestBufferMaxSizeBytes = MaxSizeBytes;
    //}

    // bool SignedFile::ReadBody()
    // Read body of signed file
    // Returns true if the format of its body is OK.
    // Otherwise returns false.
    //{
    // SignedFile makes no assumptions about the format of the
    // body of the signed file, so SignedFile::ReadBody() is a
    // null function. Override in any child classes where
    // this function is required.
    //	return true;
    //}

    static std::string read_file(const std::string& filename)
    {
        if (filename.empty())
        {
            return {};
        }
        std::ifstream ifsInput(filename.c_str(), std::ios::binary | std::ios::in);
        std::stringstream buffer;
        buffer << ifsInput.rdbuf();
        return buffer.str();
    }

    static std::vector<crypto::root_cert> read_root_certs(const string& CertFilepath, const string& CRLFilepath)
    {
        auto crl = read_file(CRLFilepath);
        crypto::root_cert root{
            .pem_crt = read_file(CertFilepath),
            .pem_crl = crl
        };
        std::vector<crypto::root_cert> root_certs;
        root_certs.push_back(root);
        return root_certs;
    }

    void SignedFile::Open(
        const string& SignedFilepath, //[i] Path to signed file
        const string& CertFilepath,   //[i] Path to CA certificate file
        const string& CRLFilepath,    //[i] Path to Certificate Revocation List file
        const bool fixDate            //[i] Fix the date of verification to work with old certs
    )
    // Open a signed file and verify its signature.
    // If all well, SignedFile status will be 'valid'.
    // Otherwise, status reflects failure.
    {
        // Test files exist
        ifstream CertFilestrm(CertFilepath.c_str(), ios::in);
        if (!CertFilestrm.is_open())
        {
            throw ve_file(notopened, CertFilepath);
        }

        if (CRLFilepath.length() > 0)
        {
            ifstream CRLFilestrm(CRLFilepath.c_str(), ios::in);
            if (!CRLFilestrm.is_open())
            {
                throw ve_file(notopened, CRLFilepath);
            }
        }

        ifstream SignedFilestrm(SignedFilepath.c_str(), ios::in | ios::binary);
        if (!SignedFilestrm.is_open())
        {
            m_Status = notopened;
            throw ve_file(notopened, SignedFilepath);
        }

        // Currently not possible to set m_DigestBufferMaxSizeBytes to anything
        // except 0 therefore the following condition is redundant.
        //	//Make digest from signed file
        //	if (m_DigestBufferMaxSizeBytes != 0)
        //	{
        //		m_DigestBuffer.set_file_body_limit(m_DigestBufferMaxSizeBytes);
        //	}

        SignedFilestrm >> m_DigestBuffer;

        if (!SignedFilestrm)
        {
            m_Status = malformed;
            throw ve_file(malformed, SignedFilepath);
        }

        // Verify digest against certificate(s)
        auto root_certs = read_root_certs(CertFilepath, CRLFilepath);
        m_DigestBuffer.verify_all(root_certs);

        // Read body and confirm format
        if (!ReadBody())
        {
            m_Status = bad_syntax;
            throw ve_file(bad_syntax, SignedFilepath);
        }

        // Everything worked ok!
        m_Status = valid;
    }

    bool SignedFile::IsValid()
    // Returns true if file properly signed
    {
        return m_Status == valid;
    }

    // SignedFile::status_enum SignedFile::Status()
    //{
    //	return m_Status;
    //}

} // namespace VerificationTool
