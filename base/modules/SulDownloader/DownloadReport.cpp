//
// Created by pair on 06/06/18.
//

#include "DownloadReport.h"
namespace SulDownloader
{

    DownloadReport DownloadReport::Report(const Warehouse &)
    {
        return DownloadReport();
    }

    DownloadReport DownloadReport::Report(const std::vector<Product> &)
    {
        return DownloadReport();
    }
}