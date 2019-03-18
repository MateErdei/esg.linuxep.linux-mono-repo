// CertificateTracker.h. This file defines a singleton class that
// is used in conjunction with an OpenSSL callback to track the names
// of any failed certificates during a verification.
//
//  20030909 RW version 1.0.0
//
//////////////////////////////////////////////////////////////////////
#if !defined(_CERTIFICATE_TRACKER_H_INCLUDED_)
#    define _CERTIFICATE_TRACKER_H_INCLUDED_

#    include <string>
#    include <vector>
using namespace std;

namespace verify_exceptions
{
    // This class is implemented as a singleton and is used to track any
    // errors during the processing of a certificate path.
    //
    // NOTE: This class is NOT thread safe. If it is used in a multi-
    // threaded environment, there will be problems reporting the correct
    // errors back to the correct thread (though the code should not fail).
    // Since the output from this class is intended to be used as an aid
    // to debugging and support, this should be OK; the information about the
    // failure should be the same in all cases and will appear somewhere in
    // log file.
    //
    class CertificateTracker
    {
        // Private constructor
        CertificateTracker()
        {
        }
        vector<string> m_CertProblems;
        // This holds the name(s) of each bad cert found.
        // This is intended as a simpler way to expose
        // the identities of any failed certs to a
        // user.
        string m_CertNames;

    public:
        // Type definition of the iterator made available
        typedef vector<string>::const_iterator iter_type;
        static CertificateTracker& GetInstance();
        void AddProblem(string TheProblem, string CertName);
        vector<string>::const_iterator GetProblems();
        vector<string>::const_iterator GetEnd();
        string GetNames()
        {
            return m_CertNames;
        }
        void Clear();
    };
} // namespace verify_exceptions
#endif
