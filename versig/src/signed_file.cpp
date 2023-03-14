// signed_file.cpp: interface for the SignedFile facade class.
//
//  20050322 Modified for SAV for Linux from verify.cpp
//
//////////////////////////////////////////////////////////////////////

#include "signed_file.h"

#include "print.h"
#include "verify_exceptions.h"

#include <cassert>
#include <filesystem>
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
        namespace fs = std::filesystem;
        std::vector<crypto::root_cert> root_certs;
        if (fs::is_regular_file(CertFilepath))
        {
            auto crl = read_file(CRLFilepath);
            crypto::root_cert root{
                .pem_crt = read_file(CertFilepath),
                .pem_crl = crl
            };
            root_certs.push_back(root);
        }
        else if (fs::is_directory(CertFilepath))
        {
            // Read certificates from directory.
            for (fs::directory_iterator it(CertFilepath); it != fs::directory_iterator(); ++it)
            {
                auto cert_path = it->path();
                if (".crt" == cert_path.extension())
                {
                    auto crl_path = cert_path;
                    crl_path.replace_extension(L".crl");
                    std::string crl;
                    if (fs::exists(crl_path))
                    {
                        crl = read_file(crl_path);
                    }

                    auto crt = read_file(cert_path);
                    root_certs.push_back(
                        {
                            .pem_crt =crt,
                            .pem_crl = crl
                        }
                    );
                }
            }
        }
        else
        {
            PRINT("Can't access Root certificates");
        }
        return root_certs;
    }

    void SignedFile::Open(const manifest::Arguments& arguments)
    // Open a signed file and verify its signature.
    // If all well, SignedFile status will be 'valid'.
    // Otherwise, status reflects failure.
    {
        const auto& CertFilepath = arguments.CertsFilepath;

            // Test files exist
        std::ifstream CertFilestrm(CertFilepath.c_str(), ios::in);
        if (!CertFilestrm.is_open())
        {
            throw ve_file(notopened, CertFilepath);
        }

        const auto& CRLFilepath = arguments.CRLFilepath;
        if (CRLFilepath.length() > 0)
        {
            ifstream CRLFilestrm(CRLFilepath.c_str(), ios::in);
            if (!CRLFilestrm.is_open())
            {
                throw ve_file(notopened, CRLFilepath);
            }
        }

        const auto& SignedFilepath = arguments.SignedFilepath;
        std::ifstream SignedFilestrm(SignedFilepath.c_str(), ios::in | ios::binary);
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
        int allowed_algorithms = crypto::SECURE_HASH_ALGORTHMS;
        if (arguments.allowSHA1signature)
        {
            PRINT("Allowing SHA1 signatures");
            allowed_algorithms |= crypto::hash_algo::ALGO_SHA1;
        }
        m_DigestBuffer.verify_all(root_certs, allowed_algorithms);

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
