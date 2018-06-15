/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_PRODUCTSELECTION_H
#define EVEREST_PRODUCTSELECTION_H

#include "ProductInformation.h"
#include "ConfigurationData.h"
#include <memory>
#include <vector>

namespace SulDownloader
{
    class ISingleProductSelector
    {
    public:
        virtual bool keepProduct ( const ProductInformation & ) const =0;
        virtual std::string targetProductName() const = 0;
        virtual ~ISingleProductSelector() = default;
    };

    class ProductSelector : public ISingleProductSelector
    {
    public:
        enum NamePrefix{UseFullName, UseNameAsPrefix};
        ProductSelector( const std::string & productPrefix , NamePrefix namePrefix, const std::string &releaseTag, const std::string &baseVersion );
        std::string targetProductName() const override ;
        bool keepProduct ( const ProductInformation & ) const override ;
        virtual ~ProductSelector() = default;
    private:
        std::string m_productName;
        NamePrefix m_NamePrefix;
        std::string m_releaseTag;
        std::string m_baseVersion;
    };

// FIXME: change ProductSelection to:
// This will provide 2 benefits: fix current bug that the Primary product is not guaranteed to be installed first.
//                               simplifies the code to keep track of missing Products in the warehouse.
//    struct Selection
//    {
//        std::vector<int> selected;
//        std::vector<int> selected;
//        std::vector<std::string> missing;
//    };
//    class ProductSelection1
//    {
//        ProductSelection1();
//    public:
//        static ProductSelection1 CreateProductSelection( const ConfigurationData & );
//        void appendSelector(std::unique_ptr<ISingleProductSelector> );
//        Selection selectProducts( const std::vector<ProductInformation> & warehouseProducts);
//    };

    /**
     * ProductSelection is responsible to define the products that must be downloaded from the warehouse as well as report
     * if any required product can not be found in the warehouse.
     */

    class ProductSelection
    {
        ProductSelection();
    public:
        static ProductSelection CreateProductSelection( const ConfigurationData & );
        void appendSelector(std::unique_ptr<ISingleProductSelector> );
        bool keepProduct ( const ProductInformation & ) const;
        std::vector<std::string> missingProduct( const std::vector<ProductInformation> & downloadedProducts) const;
    private:
        std::vector<std::unique_ptr<ISingleProductSelector>> m_selection;
        bool selectAtLeastOneProduct(ISingleProductSelector &selector,
                                     const std::vector<ProductInformation> &downloadedProducts) const;
    };



}

#endif //EVEREST_PRODUCTSELECTION_H
