// digest_body.h: interface for the digest_file_body class.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#if !defined(DIGEST_FILE_BODY_H_INCLUDED)
# define DIGEST_FILE_BODY_H_INCLUDED

#include "SophosCppStandard.h"

#include "file_info.h"

#include <iostream>
#include <list>
#include <string>

namespace VerificationTool
{
    using namespace std;
    using namespace manifest;

    // This class represents the body of a digest file.
    class digest_file_body
    {
    private:
        list<file_info> _files; // file info for each file

    public:
        typedef list<file_info>::const_iterator files_iter;

        // begin & end iterators for the collection of files
        [[nodiscard]] files_iter files_begin() const
        {
            return _files.begin();
        };

        [[nodiscard]] files_iter files_end() const
        {
            return _files.end();
        };

        friend istream& operator>>(istream& s, digest_file_body& v);
    };

} // namespace VerificationTool

// re-enable warning about very long type identifiers in debug info
//#pragma warning (default : 4786)

#endif // !defined(DIGEST_FILE_BODY_H_INCLUDED)
