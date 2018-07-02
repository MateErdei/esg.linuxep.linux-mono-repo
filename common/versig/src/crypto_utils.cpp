// crypto_utils.cpp: implementation of the cryptographic verification
// functions.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#include "crypto_utils.h"
#include "verify_exceptions.h"

#include <sstream>
#include <cassert>
#include <ctime>

#include <fstream>
#include <stdexcept>

#define PRINT(_X) std::cerr << _X << std::endl

namespace VerificationToolCrypto {

using namespace std;
using namespace verify_exceptions;

X509* X509_decode(const string &);
string BIO_read_all(BIO *bio);

// This function verifies a signature.
//
// The signed data is presented as an istream instance and
// the signature itself as a separate std::string. In addition,
// the certifiact to use in order to verify the signature is
// also supplied as an OpenSSL EVP_PKEY structure.
//
bool verify_signature(
   istream &body,             //In: Stream pointing to signed data
   const string &signature,   //In: The signature over that body
   EVP_PKEY *pubkey           //In: Public key
) {
	EVP_MD_CTX ctx;
	EVP_VerifyInit(&ctx, EVP_sha1());

   while (!body.eof()) {
		char buf[4096];         //4k buffer
		body.read(buf, 4096);
		if (body.gcount() > 0)
		{
			EVP_VerifyUpdate(&ctx, buf, body.gcount());
	}
	}

	int result = EVP_VerifyFinal(&ctx, (unsigned char*)(signature.c_str()), signature.length(), pubkey);

	(void)EVP_MD_CTX_cleanup(&ctx);

   switch (result) {
   case 1:
      return true;
   case 0:
      throw ve_badsig();
   default:
      throw ve_crypt("Error verifying signature");
   }
	// -1 for error, 0 for bad sig, 1 for verified sig
}

#ifndef NDEBUG
// This function is a utility to extract a suitable error string from an
// OpenSSL store once an error has occurred. We get a human-readable string
// from the store (this is only available in English) and access the CN part
// of the certificate name. This is used to manage logical 'errors' that crop
// up during the verification of a certificate chain (such as revoked cert,
// or expired cert) rather than actual errors (which are handled differently).
static string MakeErrString(X509_STORE_CTX* stor, string& CertName ){
   string Error;
   Error.append( X509_verify_cert_error_string(stor->error) );
   CertName.clear();
   if ( stor->current_cert )
   {
      CertName = stor->current_cert->name;
      string::size_type start = CertName.find("CN=");
      start += 3;
      string::size_type end = CertName.substr(start).find("/");
      Error.append("; ");
      CertName = CertName.substr(start, end);
      Error.append(CertName);
   }
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
static int verify_callback(int ok, X509_STORE_CTX *stor) {
   //Extract information about the current certificate
   string Error;
   if (!ok){
      string CertName;
      if ( stor != NULL ){
         Error = MakeErrString(stor, CertName);
      } else {
         Error.append("OpenSSL store not available.");
      }
      CertificateTracker::GetInstance().AddProblem( Error, CertName );
   }
	return ok;
}
#endif /* ndef NDEBUG */


// Wrapper around these X509 objects take ownership& ensure memory cleanup
struct X509StoreWrapper
{
	X509_STORE * m_ref;
	X509StoreWrapper():m_ref(X509_STORE_new()) {}
	~X509StoreWrapper(){ X509_STORE_free(m_ref);}
	X509_STORE * GetPtr() { return m_ref;}
};

struct X509StoreCtxWrapper
{	X509_STORE_CTX * m_ref;
	X509StoreCtxWrapper() : m_ref(X509_STORE_CTX_new()) {}
	~X509StoreCtxWrapper(){X509_STORE_CTX_free(m_ref); }
	X509_STORE_CTX * GetPtr() { return m_ref;}

};

struct X509StackWrapper
{
	STACK_OF(X509) *m_ref;
	X509StackWrapper() : m_ref(sk_X509_new_null()) {};
	~X509StackWrapper() { sk_X509_free(m_ref); };
	STACK_OF(X509) * GetPtr() { return m_ref;}
};

// Helper function to pull a string from a file.
static std::string SimpleLoadFile(const std::string& filename)
{
    if (filename.empty())
    {
        return std::string();
    }
    std::ifstream ifsInput(filename.c_str(), std::ios::binary | std::ios::in);

    if (!ifsInput)
    {
        PRINT("File "<<filename<<" can't be opened");
        throw ve_crypt("Bad file");
    }

    std::vector<char>inputBuffer;
    ifsInput.seekg(0, std::ios_base::end);
    std::streamoff ifsStreamSize = ifsInput.tellg();
    if (ifsStreamSize == 0)
    {
        return std::string();
    }
    else if (ifsStreamSize > 1024*1024)
    {
        PRINT("File "<<filename<<" is too large: "<<ifsStreamSize);
        throw ve_crypt("File is too large");
    }
    else if (ifsStreamSize < 0)
    {
        PRINT("File "<<filename<<" can't read stream size: "<<ifsStreamSize);
        throw ve_crypt("File size can't be read");
    }
    ifsInput.seekg(0, std::ios_base::beg);
    inputBuffer.resize(ifsStreamSize);
    char *pBuffer = &inputBuffer[0];
    ifsInput.read(pBuffer, ifsStreamSize);
    std::string output(inputBuffer.begin(), inputBuffer.end());
    ifsInput.close();
    return output;
}


// This function verifies a chain of certificates. These are
// supplied into the program in 2 ways; the first is by appending
// them to the end of the signed file itself, others are supplied
// by passing in a separate file with the trusted root cert in it.
// This latter stage ensures that we have an independent way of
// defining the trust relationship.
//
bool verify_certificate_path(
	X509 *cert,
	const string &trusted_certs_file,
	const list<X509*> &untrusted_certs,
	const string& crl_file,
	bool fixDate
)
{
    static_cast<void>(fixDate);

	// make a stack of untrusted certs
	X509StackWrapper untrusted_certs_stack;
	for (list<X509 *>::const_iterator p = untrusted_certs.begin(); p != untrusted_certs.end(); ++p)
    {
		sk_X509_push(untrusted_certs_stack.GetPtr(), *p);
    }

	X509StoreWrapper store;

	int status = 0;

    OpenSSL_add_all_digests();
    //ERR_load_crypto_strings();

#ifndef NDEBUG
    X509_STORE_set_verify_cb_func(store.GetPtr(), verify_callback);	//for debug
#endif
    std::string trusted_certs_string = SimpleLoadFile(trusted_certs_file);

#ifdef CRL_CHECKING
    std::string crl_string = SimpleLoadFile(crl_file);
    // Load all crl records

    if (!crl_string.empty())
    {
        char *data = const_cast<char *>(crl_string.c_str());
        BIO *in = BIO_new_mem_buf(data, int(crl_string.length()));
        if (!in)
        {
            throw ve_crypt("Failed to open CRL file");
        }
        int result = 1; // Can be no crl
        for (;;)
        {
            X509_CRL *crl = PEM_read_bio_X509_CRL(in,NULL,NULL,NULL);
            if (!crl)
            {
                break;
            }
            result = X509_STORE_add_crl(store.GetPtr(), crl);
            X509_CRL_free(crl);
            if (!result)
            {
                break;
            }
            X509_STORE_set_flags(store.GetPtr(), X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
        }
        BIO_free_all(in);
        if (!result)
        {
            throw ve_crypt("Failed to add CRL");
        }
    }
#else /* CRL_CHECKING */
    static_cast<void>(crl_file);
#endif /* CRL_CHECKING */

    // Load all root certificates
    char *data = const_cast<char *>(trusted_certs_string.c_str());
    BIO *in = BIO_new_mem_buf(data, int(trusted_certs_string.length()));
    if (!in)
    {
        throw ve_crypt("Error opening trusted certificates file");
    }
    int result = 0; // Must be at least one root certificate
    for (;;)
    {
        X509 *this_cert = PEM_read_bio_X509(in,NULL,NULL,NULL);
        if (!this_cert)
        {
            break;
        }
        result = X509_STORE_add_cert(store.GetPtr(), this_cert);
        X509_free(this_cert);
        if (!result)
        {
            break;
        }
    }
    BIO_free_all(in);

    if (!result)
    {
        throw ve_crypt("Error loading trusted certificates file");
    }

    // Verify against this certificate.
    X509StoreCtxWrapper verify_ctx;

    if (X509_STORE_CTX_init(verify_ctx.GetPtr(), store.GetPtr(), cert, untrusted_certs_stack.GetPtr()) != 1)
    {
        throw ve_crypt("Error initialising verification context");
    }

    status = X509_verify_cert(verify_ctx.GetPtr());
	switch (status)
	{
	case 1:
		return true;
	case 0:
		throw ve_badcert();
	default:
		throw ve_crypt("Verifying certificate");
	}
}


string BIO_read_all(BIO *bio) {
	stringstream s;
	int count;
	do {
		char buf[StringBufSize];
		count = BIO_read(bio, buf, StringBufSize);
		s.write(buf, count);
	} while (count == StringBufSize);
	return s.str();
}


string base64_decode(const string &data) {
	BIO *memfile, *b64;
	memfile = BIO_new_mem_buf(const_cast<char *>(data.c_str()), data.length());
	b64 = BIO_new(BIO_f_base64());
	BIO_push(b64, memfile);
	string result = BIO_read_all(b64);
	BIO_free_all(b64);
	return result;
}


X509* X509_decode(const string &data) {
	BIO *memfile;
	memfile = BIO_new_mem_buf(const_cast<char *>(data.c_str()), data.length());
	X509 *result = PEM_read_bio_X509(memfile, NULL, NULL, NULL);
	//allocates an X509 or returns NULL
	BIO_free(memfile);
	return result;
}

typedef string bytestring; //used as just a sequence of bytes

static bytestring sum_raw(istream &in, const EVP_MD * digest) {
	EVP_MD_CTX ctx;
	EVP_DigestInit(&ctx, digest);
	char buf[128*1024];

	while (!in.eof()) {
		in.read(buf, sizeof(buf));
		if (in.gcount() > 0)
			EVP_DigestUpdate(&ctx, buf, in.gcount());
	}

	unsigned char hash_buf[EVP_MAX_MD_SIZE];
	unsigned int hash_len = 0;
	EVP_DigestFinal(&ctx, hash_buf, &hash_len);
	//assert(hash_len != 0);

	bytestring result((char *) hash_buf, hash_len);
	return result;
}


static bytestring sha1sum_raw(istream &in) {
    return sum_raw(in, EVP_sha1());
}

static bytestring sha512sum_raw(istream &in) {
    return sum_raw(in, EVP_sha512());
}


static string hex(const bytestring &data) {
	string result;
	result.reserve(data.length() * 2); //prealocate double the space (since hex conversion doubles length)

	for (unsigned int n = 0; n < data.length(); ++n) {
		const size_t HEX_BUFFER_SIZE = 3;
		char hexbuf[HEX_BUFFER_SIZE];
		snprintf(hexbuf, HEX_BUFFER_SIZE, "%02x", (unsigned char)data[n]);
		result += hexbuf[0]; result += hexbuf[1];
	}

	return result;
}

string sha1sum(istream &in) { return hex(sha1sum_raw(in)); }

string sha512sum(istream &in) { return hex(sha512sum_raw(in)); }

unsigned int sha1size() { return EVP_MD_size(EVP_sha1()) * 2; }
unsigned int sha512size() { return EVP_MD_size(EVP_sha512()) * 2; }

} // namespace VerificationToolCrypto
