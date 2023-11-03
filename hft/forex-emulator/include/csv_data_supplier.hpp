/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2023 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual property             **
**           of LLG Ryszard Gradowski. This application as            **
**           well as any part of source code cannot be used,          **
**           modified and distributed by third party person           **
**           without prior written permission issued by               **
**           intellectual property owner.                             **
**                                                                    **
\**********************************************************************/

#ifndef __CSV_DATA_SUPPLIER_HPP__
#define __CSV_DATA_SUPPLIER_HPP__

#include <csv_loader.hpp>

#include <vector>

#include <boost/noncopyable.hpp>

class csv_data_supplier : private boost::noncopyable
{
public:

    typedef csv_loader::csv_record csv_record;

    csv_data_supplier(const std::string &file_name);

    bool get_record(csv_record &out_rec);

private:

    csv_loader csv_loader_;

    std::vector<std::string> csv_file_names_;
    std::vector<std::string>::iterator current_name_it_;
};

#endif /* __CSV_DATA_SUPPLIER_HPP__ */
