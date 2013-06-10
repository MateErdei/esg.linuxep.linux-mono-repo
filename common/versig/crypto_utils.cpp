// crypto_utils.cpp: implementation of the cryptographic verification
// functions.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#include "crypto_utils.h"
#include "verify_exceptions.h"

#include <sstream>
#include <assert.h>
#include <time.h>

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
	const string &crl_file,
	bool fixDate
) {

	// make a stack of untrusted certs
	X509StackWrapper untrusted_certs_stack;
	for (list<X509 *>::const_iterator p = untrusted_certs.begin(); p != untrusted_certs.end(); p++)
		sk_X509_push(untrusted_certs_stack.GetPtr(), *p);

	//do everything else
	X509StoreWrapper store;
	X509_LOOKUP     *lookup;
	X509StoreCtxWrapper verify_ctx;

	OpenSSL_add_all_digests();
	ERR_load_crypto_strings();

	if (!store.GetPtr()) throw ve_crypt("Error creating X509_STORE object");

#ifndef NDEBUG
	X509_STORE_set_verify_cb_func(store.GetPtr(), verify_callback);	//for debug
#endif

	if (X509_STORE_load_locations(store.GetPtr(), trusted_certs_file.c_str(), NULL) != 1)
		throw ve_crypt("Error loading trusted certificates file");

	lookup = X509_STORE_add_lookup(store.GetPtr(), X509_LOOKUP_file());
	if (!lookup) throw ve_crypt("Error creating X509_LOOKUP object");

#define EMERGENCY_CERTIFICATE_ROLLOVER_KLUDGE 1

#ifndef EMERGENCY_CERTIFICATE_ROLLOVER_KLUDGE
	if (crl_file.length() > 0) {
		int sts = X509_load_crl_file(lookup, crl_file.c_str(), X509_FILETYPE_PEM);
		if (sts ==0)
         throw ve_file( SignedFile::notopened, crl_file );

#if (OPENSSL_VERSION_NUMBER > 0x00907000L)
		X509_STORE_set_flags(store.GetPtr(), X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
#else
#warning No CRL checking will be performed, that requires OpenSSL 0.9.7 or later
#ifndef NDEBUG
		cerr << "No CRL checking will be performed, that requires OpenSSL 0.9.7 or later" << endl;
#endif
#endif
	}
#else
    static_cast<void>(crl_file);
#endif

	if (!verify_ctx.GetPtr()) throw ve_crypt("Error creating X509_STORE_CTX object");

#if (OPENSSL_VERSION_NUMBER > 0x00907000L)
	if (X509_STORE_CTX_init(verify_ctx.GetPtr(), store.GetPtr(), cert, untrusted_certs_stack.GetPtr()) != 1)
		throw ve_crypt("Error initialising verification context");
#else
	X509_STORE_CTX_init(verify_ctx.GetPtr(), store.GetPtr(), cert, untrusted_certs_stack);
#endif

#ifdef EMERGENCY_CERTIFICATE_ROLLOVER_KLUDGE
    if (fixDate)
    {
	struct tm jan_first_tm;
	jan_first_tm.tm_sec = 0;
	jan_first_tm.tm_min = 0;
	jan_first_tm.tm_hour = 0;
	jan_first_tm.tm_mday = 1;
	jan_first_tm.tm_mon = 0;
	jan_first_tm.tm_year = 109;
	jan_first_tm.tm_wday = 0;
	jan_first_tm.tm_yday = 0;
	jan_first_tm.tm_isdst = 0;

//struct tm {
//        int tm_sec;     /* seconds after the minute - [0,59] */
//        int tm_min;     /* minutes after the hour - [0,59] */
//        int tm_hour;    /* hours since midnight - [0,23] */
//        int tm_mday;    /* day of the month - [1,31] */
//        int tm_mon;     /* months since January - [0,11] */
//        int tm_year;    /* years since 1900 */
//        int tm_wday;    /* days since Sunday - [0,6] */
//        int tm_yday;    /* days since January 1 - [0,365] */
//        int tm_isdst;   /* daylight savings time flag */
//        };

// /* Used by other time functions.  */
// struct tm
// {
//   int tm_sec;                   /* Seconds.     [0-60] (1 leap second) */
//   int tm_min;                   /* Minutes.     [0-59] */
//   int tm_hour;                  /* Hours.       [0-23] */
//   int tm_mday;                  /* Day.         [1-31] */
//   int tm_mon;                   /* Month.       [0-11] */
//   int tm_year;                  /* Year - 1900.  */
//   int tm_wday;                  /* Day of week. [0-6] */
//   int tm_yday;                  /* Days in year.[0-365] */
//   int tm_isdst;                 /* DST.         [-1/0/1]*/

// #ifdef  __USE_BSD
//   long int tm_gmtoff;           /* Seconds east of UTC.  */
//   __const char *tm_zone;        /* Timezone abbreviation.  */
// #else
//   long int __tm_gmtoff;         /* Seconds east of UTC.  */
//   __const char *__tm_zone;      /* Timezone abbreviation.  */
// #endif
// };

	time_t january_the_first = mktime(&jan_first_tm);
	X509_STORE_CTX_set_time(verify_ctx.GetPtr(), 0, january_the_first);
    }
#endif

	int status = X509_verify_cert(verify_ctx.GetPtr());
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

static bytestring sha1sum_raw(istream &in) {
        EVP_MD_CTX ctx;
	EVP_DigestInit(&ctx, EVP_sha1());
	while (!in.eof()) {
		char buf[4096];         //4k buffer
		in.read(buf, 4096);
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


static string hex(const bytestring &data) {
	string result;
	result.reserve(data.length() * 2); //prealocate double the space (since hex conversion doubles length)

	for (unsigned int n = 0; n < data.length(); n++) {
		char hexbuf[3];
		snprintf(hexbuf, 3, "%02x", (unsigned char)data[n]);
		result += hexbuf[0]; result += hexbuf[1];
	}

	return result;
}

string sha1sum(istream &in) { return hex(sha1sum_raw(in)); }

} // namespace VerificationToolCrypto
