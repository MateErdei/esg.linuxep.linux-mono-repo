//
// Created by pair on 06/06/18.
//

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
        virtual ~ISingleProductSelector() = default;
    };

    class ProductSelector : public ISingleProductSelector
    {
    public:
        enum NamePrefix{UseFullName, UseNameAsPrefix};
        ProductSelector( const std::string & productPrefix , NamePrefix namePrefix, const std::string &releaseTag, const std::string &baseVersion );
        bool keepProduct ( const ProductInformation & ) const override ;
        virtual ~ProductSelector() = default;
    private:
        std::string m_productName;
        NamePrefix m_NamePrefix;
        std::string m_releaseTag;
        std::string m_baseVersion;
    };

    class ProductSelection
    {
        ProductSelection();
    public:
        static ProductSelection CreateProductSelection( const ConfigurationData & );
        void appendSelector(std::unique_ptr<ISingleProductSelector> );
        bool keepProduct ( const ProductInformation & ) const;
    private:
        std::vector<std::unique_ptr<ISingleProductSelector>> m_selection;
    };



}

#endif //EVEREST_PRODUCTSELECTION_H
