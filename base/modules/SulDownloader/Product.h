//
// Created by pair on 05/06/18.
//

#ifndef SULDOWNLOADER_PRODUCTS_H
#define SULDOWNLOADER_PRODUCTS_H

#include "ProductInformation.h"
#include "SULUtils.h"

namespace SulDownloader
{
    class Product
    {
    public:
        explicit Product(  ProductInformation  );
        bool verify();

        void install();
        bool hasError() const;
        void setError( const std::string & );
        Error getError() const;
        std::string distributionFolderName();
        bool setDistributePath(const std::string & distributePath);
        void verifyDistributionStatus();

    private :
        enum class State{ Initialized, Distributed, Verified, Installed, HasError} m_state;
        Error m_error;
        ProductInformation m_productInformation;
        std::string m_distributePath;
    };
}

#endif //SULDOWNLOADER_PRODUCTS_H
