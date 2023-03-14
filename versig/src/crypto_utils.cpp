// crypto_utils.cpp: implementation of the cryptographic verification
// functions.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#include "crypto_utils.h"

#include "libcryptosupport.h"
#include "verify_exceptions.h"

#include <cstdio>
#include <sstream>
#include <stdexcept>

namespace VerificationToolCrypto
{
    using namespace std;
    using namespace verify_exceptions;

    // X509* X509_decode(const string &);
    string BIO_read_all(BIO* bio);

#ifndef NDEBUG
    std::string convertASN1StringToStdString(ASN1_STRING* d)
    {
        std::string asn1_string;
        size_t length;
        if (ASN1_STRING_type(d) != V_ASN1_UTF8STRING)
        {
            unsigned char* utf8;
            length = static_cast<size_t>(ASN1_STRING_to_UTF8(&utf8, d));
            asn1_string = std::string(reinterpret_cast<char*>(utf8), static_cast<size_t>(length));
            OPENSSL_free(utf8);
        }
        else
        {
            length = static_cast<size_t>(ASN1_STRING_length(d));
            asn1_string = std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(d)), length);
        }
        return asn1_string;
    }

    // This function is a utility to extract a suitable error string from an
    // OpenSSL store once an error has occurred. We get a human-readable string
    // from the store (this is only available in English) and access the CN part
    // of the certificate name. This is used to manage logical 'errors' that crop
    // up during the verification of a certificate chain (such as revoked cert,
    // or expired cert) rather than actual errors (which are handled differently).
    static string MakeErrString(X509_STORE_CTX* stor, string& CertName)
    {
        string Error;
        Error.append(X509_verify_cert_error_string(X509_STORE_CTX_get_error(stor)));
        CertName.clear();
        X509* currentCert = X509_STORE_CTX_get_current_cert(stor);
        if (currentCert == NULLPTR)
        {
            return Error;
        }
        X509_NAME* nm = X509_get_subject_name(currentCert);
        if (nm == NULLPTR)
        {
            return Error;
        }
        int lastpos = -1;

        for (;;)
        {
            lastpos = X509_NAME_get_index_by_NID(nm, NID_commonName, lastpos);
            if (lastpos == -1)
                break;
            X509_NAME_ENTRY* e = X509_NAME_get_entry(nm, lastpos);
            /* Do something with e */
            ASN1_STRING* common_name_asn1 = X509_NAME_ENTRY_get_data(e);
            CertName = convertASN1StringToStdString(common_name_asn1);
        }
        Error.append("; ");
        Error.append(CertName);
        return Error;
    }

    // This function is a callback supplied to the OpenSSL library. We get called
    // each time a certificate is handled during the verification process. If there
    // is a problem with a certificate we can spot this and extract details about the
    // error and the certificate. This function uses MakeErrString above to help
    // it extract information from the certificate.
    // Note. The OpenSSL errors are only in English so these messages are NOT
    // suitable for exposure to a user. They are, however, essential for
    // debugging/support purposes, so this code remains to extract the details.
    //
    static int verify_callback(int ok, X509_STORE_CTX* stor)
    {
        // Extract information about the current certificate
        string Error;
        if (!ok)
        {
            string CertName;
            if (stor != NULLPTR)
            {
                Error = MakeErrString(stor, CertName);
            }
            else
            {
                Error.append("OpenSSL store not available.");
            }
            CertificateTracker::GetInstance().AddProblem(Error, CertName);
        }
        return ok;
    }
#endif /* ndef NDEBUG */

    string BIO_read_all(BIO* bio)
    {
        stringstream s;
        int count;
        do
        {
            char buf[StringBufSize];
            count = BIO_read(bio, buf, StringBufSize);
            s.write(buf, count);
        } while (count == StringBufSize);
        return s.str();
    }

    string base64_decode(const string& data)
    {
        crypto::BIOWrapper memfile(data);
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO_push(b64, memfile.release());
        string result = BIO_read_all(b64);
        BIO_free_all(b64);
        return result;
    }

    X509* X509_decode(const string& data)
    {
        crypto::BIOWrapper memfile(data);
        // allocates an X509 or returns NULL
        return PEM_read_bio_X509(memfile.get(), NULLPTR, NULLPTR, NULLPTR);
    }

    typedef string bytestring; // used as just a sequence of bytes

    static bytestring sum_raw(istream& in, const EVP_MD* digest)
    {
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        EVP_DigestInit(ctx, digest);
        char buf[128 * 1024];

        while (!in.eof())
        {
            in.read(buf, sizeof(buf));
            if (in.gcount() > 0)
                EVP_DigestUpdate(ctx, buf, in.gcount());
        }

        unsigned char hash_buf[EVP_MAX_MD_SIZE];
        unsigned int hash_len = 0;
        EVP_DigestFinal(ctx, hash_buf, &hash_len);
        EVP_MD_CTX_free(ctx);
        // assert(hash_len != 0);

        bytestring result((char*)hash_buf, hash_len);
        return result;
    }

    static bytestring sha1sum_raw(istream& in)
    {
        return sum_raw(in, EVP_sha1());
    }

    static bytestring sha512sum_raw(istream& in)
    {
        return sum_raw(in, EVP_sha512());
    }

    static bytestring sha256sum_raw(istream& in)
    {
        return sum_raw(in, EVP_sha256());
    }

    static string hex(const bytestring& data)
    {
        string result;
        result.reserve(data.length() * 2); // preallocate double the space (since hex conversion doubles length)

        for (unsigned int n = 0; n < data.length(); ++n)
        {
            const size_t HEX_BUFFER_SIZE = 3;
            char hexbuf[HEX_BUFFER_SIZE];
            snprintf(hexbuf, HEX_BUFFER_SIZE, "%02x", (unsigned char)data[n]);
            result += hexbuf[0];
            result += hexbuf[1];
        }

        return result;
    }

    string sha1sum(istream& in)
    {
        return hex(sha1sum_raw(in));
    }

    string sha512sum(istream& in)
    {
        return hex(sha512sum_raw(in));
    }

    string sha256sum(istream& in)
    {
        return hex(sha256sum_raw(in));
    }

    string sha384sum(istream& in)
    {
        return hex(sum_raw(in, EVP_sha384()));
    }

    unsigned int sha1size()
    {
        return EVP_MD_size(EVP_sha1()) * 2;
    }
    unsigned int sha512size()
    {
        return EVP_MD_size(EVP_sha512()) * 2;
    }

} // namespace VerificationToolCrypto

namespace crypto
{

    std::string hex(const std::string& data)
    {
        std::string result;
        result.reserve(data.length() * 2); //preallocate double the space (since hex conversion doubles length)

        for (size_t n = 0; n < data.length(); n++)
        {
            const size_t HEX_BUFFER_SIZE = 3;
            char hexbuf[HEX_BUFFER_SIZE];
            snprintf(hexbuf, HEX_BUFFER_SIZE, "%02x", (unsigned char)data[n]);
            result += hexbuf[0]; result += hexbuf[1];
        }

        return result;
    }
}
