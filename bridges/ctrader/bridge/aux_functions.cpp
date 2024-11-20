/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2024 by LLG Ryszard Gradowski          **
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

#include <chrono>
#include <cctype>
#include <iomanip>
#include <sstream>

#include <aux_functions.hpp>

namespace aux {

namespace {

std::string time_duration2string(boost::posix_time::time_duration d)
{
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-truncation"

    char buff[13]; // Size of ‘hh:mm:ss.fff’ plus null character.

    int frac = 1000.0*((double) d.fractional_seconds() / 1000000.0);

    snprintf(buff, sizeof(buff), "%02d%c%02d%c%02d%c%03d", (int) d.hours(), ':', (int) d.minutes(), ':', (int) d.seconds(), '.', frac);

    return buff;
    #pragma GCC diagnostic pop
}

} // namespace

unsigned long get_current_timestamp(void)
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    return millis;
}

unsigned long ptime2timestamp(const boost::posix_time::ptime &t)
{
    static boost::posix_time::ptime begin { boost::gregorian::date(1970, boost::date_time::Jan, 1) };

    return (t - begin).total_milliseconds();
}

std::string timestamp2string(unsigned long timestamp)
{
    using namespace boost::posix_time;

    ptime pt { boost::gregorian::date(1970, boost::date_time::Jan, 1) };
    pt = pt + milliseconds(timestamp);

    return boost::gregorian::to_iso_extended_string(pt.date()) + " "
           + time_duration2string(pt.time_of_day());
}

std::string hexdump(const std::string &data)
{
    std::ostringstream dump;

    auto buf = &data.front();
    int buflen = data.size();

    dump << std::setfill('0');

    for (int i = 0; i < buflen; i += 16)
    {
        dump << std::internal << std::setw(6) << std::hex << i << ": ";

        for (int j = 0; j < 16; j++)
        {
            if (i+j < buflen)
            {
                dump << std::internal << std::setw(2) << std::hex << ((unsigned int) ((unsigned char)buf[i+j])) << ' ';
            }
            else
            {
                dump << "   ";
            }
        }

        dump << " ";

        for (int j = 0; j < 16; j++)
        {
            if (i+j < buflen)
            {
                dump << (isprint(buf[i+j]) ? buf[i+j] : '.');
            }
        }

        dump << std::endl;
    }

    return dump.str();
}

} // namespace aux
