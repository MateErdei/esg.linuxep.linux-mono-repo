/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef SULDOWNLOADER_PRODUCTS_H
#define SULDOWNLOADER_PRODUCTS_H

#include "ProductInformation.h"
#include "WarehouseError.h"
namespace SulDownloader
{
    /**
     * Refer to Downloaded product from the warehouse.
     * It created by the Warehouse, which also is responsible to set its DistributePath.
     *
     * After being fully configured by the Warehouse, it must:
     *   - verify: versig verification
     *   - install: run the product installer.
     *
     * Design decision: Similar to the warehouse, Product mark the errors internally to
     * enable DownloadReport to create its report.
     */
    class DownloadedProduct
    {
    public:
        explicit DownloadedProduct(  ProductInformation  );
        bool verify();

        void install(const std::vector<std::string> & installArgs);
        bool hasError() const;
        //void setError( const std::string & );
        void setError( WarehouseError error);
        WarehouseError getError() const;
        std::string distributionFolderName();
        void setDistributePath(const std::string & distributePath);
        std::string distributePath() const;
        ProductInformation getProductInformation();
        std::string getLine() const;
        std::string getName() const;
        bool productHasChanged() const;
        void setProductHasChanged( bool  );
        std::string getPostUpdateInstalledVersion() const;
        std::string getPreUpdateInstalledVersion() const ;
        void setPreUpdateInstalledVersion(const std::string & preUpdateInstalledVersion);
        void setPostUpdateInstalledVersion(const std::string & postUpdateInstalledVersion);


    private :
        enum class State{ Initialized, Distributed, Verified, Installed, HasError} m_state;
        WarehouseError m_error;
        ProductInformation m_productInformation;
        std::string m_distributePath;
        bool m_productHasChanged;

        std::string m_postUpdateInstalledVersion;
        std::string m_preUpdateInstalledVersion;

    };
}

#endif //SULDOWNLOADER_PRODUCTS_H
