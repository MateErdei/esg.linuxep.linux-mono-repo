/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ConfigurationData.h"
#include "ProductMetadata.h"
#include "WarehouseError.h"

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        /**
         * Refer to Downloaded product from the WarehouseRepository.
         * It is created by the WarehouseRepository, which also is responsible to set its DistributePath.
         *
         * After being fully configured by the WarehouseRepository, it must:
         *   - verify: versig verification
         *   - install: run the product installer.
         *
         * Design decision: Similar to the WarehouseRepository, DownloadedProduct mark the errors internally to
         * enable DownloadReport to create its report.
         */
        class DownloadedProduct
        {
        public:
            explicit DownloadedProduct(const ProductMetadata&);

            /**
             * Perform a versig verification.
             * @param configurationData: path to the rootca.crt certificate.
             * @note If the verification fails, internal error will be set and can be checked by hasError.
             * @pre ::setDistributePath called first and ::hasError return false.
             */
            void verify(const ConfigurationData& configurationData);

            /**
             * Run the installer that should be in ::distributePath() + /install.sh.
             * Passing the installArgs to the installer
             *
             * @param installArgs
             *
             * @note If the installer fails, the internal error will be set. It can be checked with the hasError.
             * @pre ::verify called first and ::hasError return false.
             */
            void install(const std::vector<std::string>& installArgs);

            /**
             *
             * @return true if any error was set for the DownloadedProduct
             */
            bool hasError() const;

            /**
             * Set description of a failure related to the DownloadedProduct. This is used either internaly when
             * ::verify and ::install is called. But can also be called by the WarehoureRepository to signal error in
             * distributing the product.
             *
             * @param error Description of the Failure related to the product.
             */
            void setError(const WarehouseError& error);

            WarehouseError getError() const;

            /**
             * The Distribute path is the local directory where the product files and the installer is found.
             * Install uses this path in order to work out the expected installer path and execute it.
             *
             * This method is to be used by the WarehouseRepository and must be executed before ::verify or ::install.
             *
             * @param distributePath path to the local directory where the product is to be found to run the installer.
             */
            void setDistributePath(const std::string& distributePath);

            /**
             *
             * @return The path configured by setDistributePath.
             */
            const std::string& distributePath() const;

            /**
             *
             * @return The ProductMetadata associated with the current DownloadedProduct.
             */
            const ProductMetadata& getProductMetadata() const;

            /**
             * Auxiliar method to return the product line. It is equivalent to: getProductMetadata().getLine()
             * @return Product Line
             */
            const std::string& getLine() const;

            /**
             *
             * @return Flag configured by ::setProductHasChanged
             */
            bool productHasChanged() const;

            /**
             * Set by the WarehouseRepository to signal that at the distribution the WarehouseRepository found that
             * there are updates for the downloaded product.
             *
             * Internally nothing is processed. The flag is just kept and returned on productHasChanged.
             *
             * This is intended to be used to check if install must be executed or not during an product update.
             *
             * @param hasChanged True if there are changes. False otherwise.
             */
            void setProductHasChanged(bool hasChanged);

            /**
             *
             * @param isBeingUninstalled True if the product is being uninstalled false if the product has been
             * installed.
             */
            void setProductIsBeingUninstalled(bool IsBeingUninstalled);

            /**
             *
             * @return true if the product is being uninstalled false otherwise.
             */
            bool getProductIsBeingUninstalled() const;

            /**
             * set forceReInstall if product failed to successfully install on the previous update
             * @param forceRinstall
             */
            void setForceProductReinstall(bool forceReinstall);

            /**
             * @return true of the product needs to be reinstalled, false otherwise
             */
            bool forceProductReinstall() const;

        private:
            enum class State
            {
                Initialized,
                Distributed,
                Verified,
                Installed,
                HasError
            } m_state;
            WarehouseError m_error;
            ProductMetadata m_productMetadata;
            std::string m_distributePath;
            bool m_productHasChanged;
            bool m_productUninstall;
            bool m_forceProductReinstall;
        };
    } // namespace suldownloaderdata
} // namespace SulDownloader
