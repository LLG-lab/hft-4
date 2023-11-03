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

#include <csv_data_supplier.hpp>

#include <boost/algorithm/string.hpp>

#include <sstream>

csv_data_supplier::csv_data_supplier(const std::string &file_name)
{
    if (boost::iends_with(file_name, ".csv"))
    {
        csv_file_names_.push_back(file_name);
    }
    else
    {
        std::ifstream infile;
        std::string line;

        infile.open(file_name.c_str(), std::ifstream::in);

        if (! infile.is_open())
        {
            std::ostringstream error_msg;

            error_msg << "Unable to open file: ‘" << file_name << "’";

            throw std::runtime_error(error_msg.str());
        }

        while (std::getline(infile, line))
        {
            boost::trim(line);

            if (line.length() > 0)
            {
                csv_file_names_.push_back(line);
            }
        }
    }

    current_name_it_ = csv_file_names_.begin();
    csv_loader_.load(*current_name_it_);
}

bool csv_data_supplier::get_record(csv_record &out_rec)
{
    while (true)
    {
        if (csv_loader_.get_record(out_rec))
        {
            return true;
        }

        current_name_it_++;

        if (current_name_it_ == csv_file_names_.end())
        {
            return false;
        }

        csv_loader_.load(*current_name_it_);
    }
}
