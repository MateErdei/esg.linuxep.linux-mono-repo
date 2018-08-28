/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "IVersig.h"
#include <functional>
namespace SulDownloader
{
    class VersigImpl: public virtual suldownloaderdata::IVersig
    {
    public:
        VerifySignature verify( const std::string & certificate_path, const std::string & productDirectoryPath ) const override ;
    };

    using IVersigPtr = suldownloaderdata::IVersigPtr;
    using VersigCreatorFunc = std::function<IVersigPtr(void)>;

    class VersigFactory
    {
        VersigFactory();
        VersigCreatorFunc m_creator;
    public:
        static VersigFactory & instance();
        IVersigPtr createVersig();
        // for tests only
        void replaceCreator(VersigCreatorFunc creator);
        void restoreCreator();
    };
}


