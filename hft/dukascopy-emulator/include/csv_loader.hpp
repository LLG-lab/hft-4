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

#ifndef __CSV_LOADER_HPP__
#define __CSV_LOADER_HPP__

#include <fstream>
#include <stdexcept>

#include <boost/noncopyable.hpp>

#include <custom_except.hpp>

class csv_loader : private boost::noncopyable
{
public:

    DEFINE_CUSTOM_EXCEPTION_CLASS(csv_exception, std::runtime_error)

    struct csv_record
    {
        csv_record(void)
            : ask(0.0), bid(0.0),
              ask_volume(0.0), bid_volume(0.0) {}

        std::string request_time;
        double ask;
        double bid;
        double ask_volume;
        double bid_volume;
    };

    csv_loader(void) = default;
    csv_loader(const std::string &file_name);
    ~csv_loader(void);

    void load(const std::string &file_name);

    bool get_record(csv_record &out_rec);

    //
    // Auxiliary methods for rewind purposes.
    //

    long get_record_position(void) const;
    void set_record_position(long position);

private:

    static int validate_range(const char *topic, int value, int min, int max);

    enum
    {
        CSV_DATE = 0,
        CSV_ASK,
        CSV_BID,
        CSV_ASK_VOLUME,
        CSV_BID_VOLUME,
        CSV_TOTAL_COLUMNS
    };

    enum /* 01.09.2015 00:00:00.016 */
    {
        CSV_DATETIME_DAY = 0,
        CSV_DATETIME_MONTH,
        CSV_DATETIME_YEAR,
        CSV_DATETIME_HOUR,
        CSV_DATETIME_MINUTE,
        CSV_DATETIME_SECOND,
        CSV_DATETIME_MILLIS,
        CSV_DATETIME_TOTAL_ITEMS
    };

    mutable std::ifstream infile_;
};

#endif /* __CSV_LOADER_HPP__ */
