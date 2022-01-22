/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2022 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual propery              **
**           of LLG Ryszard Gradowski. This application as            **
**           well as any part of source code cannot be used,          **
**           modified and distributed by third party person           **
**           without prior written permission issued by               **
**           intellectual property owner.                             **
**                                                                    **
\**********************************************************************/

#include <chrono>

#include <aux_functions.hpp>

namespace aux {

unsigned long get_current_timestamp(void)
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    return millis;
}

unsigned long ptime2timestamp(const boost::posix_time::ptime &t)
{
    static boost::posix_time::ptime begin = boost::posix_time::ptime(boost::posix_time::time_from_string("1970-01-01 00:00:00.000"));

    return (t - begin).total_milliseconds();
}

} // namespace aux
