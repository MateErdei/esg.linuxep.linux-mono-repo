/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "ProductMetadata.h"
#include "ConfigurationData.h"
#include <memory>
#include <vector>

namespace SulDownloader
{
    class ISingleProductSelector
    {
    public:
        virtual bool keepProduct ( const ProductMetadata & ) const =0;
        virtual std::string targetProductName() const = 0;
        virtual ~ISingleProductSelector() = default;
    };

    class ProductSelector : public virtual ISingleProductSelector
    {
    public:
        enum NamePrefix{UseFullName, UseNameAsPrefix};
        ProductSelector( const std::string & productPrefix , NamePrefix namePrefix, const std::string &releaseTag, const std::string &baseVersion );
        std::string targetProductName() const override ;
        bool keepProduct ( const ProductMetadata & ) const override ;
        virtual ~ProductSelector() = default;
    private:
        std::string m_productName;
        NamePrefix m_NamePrefix;
        std::string m_releaseTag;
        std::string m_baseVersion;
    };

    struct SelectedResultsIndexes
    {
        std::vector<size_t> selected;
        std::vector<size_t> notselected;
        std::vector<std::string> missing;

    };


    /**
     * ProductSelection is responsible to define the products that must be downloaded from the WarehouseRepository as well as report
     * if any required product can not be found in the remote warehouse repository.
     */

    class ProductSelection
    {
        ProductSelection() = default;
    public:
        static ProductSelection CreateProductSelection( const ConfigurationData & );
        void appendSelector(std::unique_ptr<ISingleProductSelector> );
        SelectedResultsIndexes selectProducts( const std::vector<ProductMetadata> & warehouseProducts) const;
    private:
        std::vector<std::unique_ptr<ISingleProductSelector>> m_selection;
        std::vector<size_t> selectedProducts( const ISingleProductSelector & , const std::vector<ProductMetadata> & warehouseProducts ) const;
    };



}


