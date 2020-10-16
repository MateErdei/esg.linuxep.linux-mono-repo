/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <SulDownloader/suldownloaderdata/CatalogueInfo.h>
#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <SulDownloader/suldownloaderdata/ConnectionSelector.h>
#include <SulDownloader/suldownloaderdata/IWarehouseRepository.h>
#include <SulDownloader/suldownloaderdata/Tag.h>
#include <SulDownloader/suldownloaderdata/WarehouseError.h>

extern "C"
{
#include <SUL.h>
}
#include <memory>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class DownloadedProduct;
        class ProductSelection;
    } // namespace suldownloaderdata
    class SULSession;

    /**
     * The class that encapsulate the functionalities provided by SULlib.
     *
     * The common procedure is to :
     *   - FetchConnectedWarehouse which
     *     - connect to the remote warehouse repository
     *     - download the metadata
     *   - synchronize:
     *     - remove the unwanted products
     *     - sync
     *   - distribute:
     *     - distribute and check status.
     *
     * Design Decision: set internal error state (not throw). This is to allow the creation of DownloadReport
     * from all the information kept in the WarehouseRepository object. This imply that after the main methods:
     * FetchConnectedWarehouse, synchronize and distribute is called, the ::hasError must be checked and no further
     * operation is allowed.
     *
     * The order for the methods to be called: Fetch, Synchronize and Distribute is enforced internally with asserts.
     * It also enforces that no main method is called after internal error is set.
     *
     *
     */
    class WarehouseRepository : public virtual suldownloaderdata::IWarehouseRepository
    {
    public:
        /**
         * Attempt to connect to a provided connection setup information.
         *
         *
         * @param connectionSetup
         * @param supplementOnly  Only download supplements
         * @param sulLogs  Append logs to this object
         * @param configurationData
         * @return
         */
        static std::unique_ptr<WarehouseRepository> tryConnect(
            const suldownloaderdata::ConnectionSetup& connectionSetup,
            bool supplementOnly,
            SulLogsVector& sulLogs,
            const suldownloaderdata::ConfigurationData& configurationData);

        /**
         * Create a failed warehouse object, logging sulLogs in the process
         * @param sulLogs
         * @return
         */
        static std::unique_ptr<WarehouseRepository> createFailedWarehouse(
            SulLogsVector& sulLogs
            );

        static void dumpLogs(const SulLogsVector& sulLogs);
        void dumpLogs() const override;

        WarehouseRepository(const WarehouseRepository&) = delete;
        WarehouseRepository& operator=(const WarehouseRepository&) = delete;
        WarehouseRepository& operator=(WarehouseRepository&&) = default;
        WarehouseRepository(WarehouseRepository&&) = default;
        ~WarehouseRepository() override;

        /**
         * Attempt to connect to a provided connection setup information.
         *
         *
         * @param connectionSetup
         * @param supplementOnly  Only download supplements
         * @param configurationData
         * @return
         */
        bool tryConnect(
            const suldownloaderdata::ConnectionSetup& connectionSetup,
            bool supplementOnly,
            const suldownloaderdata::ConfigurationData& configurationData) override;

        SulLogsVector getLogs() const
        {
            return m_sulLogs;
        }

        /**
         * Used to check if the WarehouseRepository reported an error
         * @return true, if WarehouseRepository has error, false otherwise
         */
        bool hasError() const override;

        /**
         * Gets the WarehouseRepository error information.
         *
         * Mainly used by DownloadReport to create its report.
         *
         * @return struct containing the error description, sul error and status
         */
        suldownloaderdata::WarehouseError getError() const override;

        /**
         * Configure sul to download/synchronize the selection of products
         *
         * Based on the list of products available in the warehouse repositories, this method will configure SUL to
         * download only the prooducts selected by ::productSelection. It will eventually execute SU_synchronise which
         * will ensure that all the packages that need to be downloaded are fetched and placed in the local repository.
         *
         * @param productSelection: is responsible to define which are the products to be downloaded.
         */
        void synchronize(suldownloaderdata::ProductSelection& productSelection) override;

        /**
         * Extract the file content from the local warehouse repository and create real product file structure required.
         * SUL works with 2 local repositories. The first one, (cache repository) allows SUL to synchronize with the
         * remote repositories. The second one, which SUL calls the distribution path, is used to synchronize inside the
         * machine and allows SUL to check if there are local changes since the previous update.
         *
         * Distribute eventually executes SU_distribute to allow SUL to verify which products have changed since the
         * last update.
         */
        void distribute() override;

        /**
         * Gets the list of products synchronized and downloaded.
         *
         * In normal operation is used after distribute, to validate and install the products.
         * It is also used by DownloadReport in order to create its report.
         *
         * @return list of products
         */
        std::vector<suldownloaderdata::DownloadedProduct> getProducts() const override;

        std::vector<suldownloaderdata::ProductInfo> listInstalledProducts() const override;
        std::vector<suldownloaderdata::SubscriptionInfo> listInstalledSubscriptions() const override;

        std::string getSourceURL() const override;

        std::string getProductDistributionPath(const suldownloaderdata::DownloadedProduct&) const override;

        WarehouseRepository();
    private:
        explicit WarehouseRepository(bool createSession);

        enum class State
        {
            Initialized,
            Failure,
            Synchronized,
            Connected,
            Distributed
        } m_state;

        void setError(const std::string&);
        void setConnectionSetup(
            const suldownloaderdata::ConnectionSetup& connectionSetup,
            const suldownloaderdata::ConfigurationData& configurationData);
        int logLevel(suldownloaderdata::ConfigurationData::LogLevel);

        void distributeProduct(
            std::pair<SU_PHandle, suldownloaderdata::DownloadedProduct>& productPair,
            const std::string& distributePath);
        void verifyDistributeProduct(std::pair<SU_PHandle, suldownloaderdata::DownloadedProduct>& productPair);

        suldownloaderdata::WarehouseError fetchSulError(const std::string& description) const;

        std::string getRootDistributionPath() const;
        void setRootDistributionPath(const std::string& rootDistributionPath);

        SU_Handle session() const;
        suldownloaderdata::WarehouseError m_error;
        std::vector<std::pair<SU_PHandle, suldownloaderdata::DownloadedProduct>> m_products;
        std::unique_ptr<SULSession> m_session;
        std::unique_ptr<suldownloaderdata::ConnectionSetup> m_connectionSetup;
        std::string m_rootDistributionPath;
        suldownloaderdata::CatalogueInfo m_catalogueInfo;
        std::vector<suldownloaderdata::SubscriptionInfo> m_selectedSubscriptions;
        bool m_supplementOnly = false;
        SulLogsVector m_sulLogs;
    };

} // namespace SulDownloader
