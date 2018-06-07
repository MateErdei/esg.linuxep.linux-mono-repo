//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_DOWNLOADREPORT_H
#define EVEREST_DOWNLOADREPORT_H

#include <vector>

namespace SulDownloader
{
    class Warehouse;
    class Product;
    class DownloadReport
    {
    public:
        static DownloadReport Report( const Warehouse & );
        static DownloadReport Report(const std::vector<Product> & );
    };

}


#endif //EVEREST_DOWNLOADREPORT_H
