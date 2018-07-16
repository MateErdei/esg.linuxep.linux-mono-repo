/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IVERSIG_H
#define EVEREST_BASE_IVERSIG_H
#include <string>
#include <memory>
namespace SulDownloader
{
    class IVersig
    {
    public:
        virtual ~IVersig() = default;
        enum class VerifySignature {SIGNATURE_VERIFIED, SIGNATURE_FAILED, INVALID_ARGUMENTS};
        virtual VerifySignature verify( const std::string & certificatePath, const std::string & productDirectoryPath ) const = 0 ;
    };

    std::unique_ptr<IVersig> createVersig();



}
#endif //EVEREST_BASE_IVERSIG_H
