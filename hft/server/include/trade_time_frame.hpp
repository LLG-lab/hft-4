/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2021 by LLG Ryszard Gradowski          **
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

#ifndef __TRADE_TIME_FRAME_HPP__
#define __TRADE_TIME_FRAME_HPP__

#include <stdexcept>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/json.hpp>


class trade_time_frame
{
public:

    trade_time_frame(void);
    ~trade_time_frame(void) = default;

    void init_from_json_object(const boost::json::object &ttf_config);

    bool can_play(const boost::posix_time::ptime &current_time_point) const;

private:

    enum
    {
        SUN, MON, TUE, WED, THU, FRI, SAT
    };

    struct timeframe_restriction
    {
        boost::posix_time::time_duration trade_period_from;
        boost::posix_time::time_duration trade_period_to;
    };

    static timeframe_restriction parse_restriction(std::string restriction);

    static bool can_play_today(const timeframe_restriction &restriction,
                                   const boost::posix_time::time_duration &td);

    static const char *const days_[];

    timeframe_restriction tfr_[7];
};

#endif /* __TRADE_TIME_FRAME_HPP__ */
