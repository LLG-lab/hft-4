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

#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <trade_time_frame.hpp>

//
// In production it should be undefined.
//

#undef TF_DEBUG

#ifdef TF_DEBUG

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, "trade_time_frame")

#endif //TF_DEBUG

const char *const trade_time_frame::days_[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

trade_time_frame::trade_time_frame(void)
{
    #ifdef TF_DEBUG
    el::Loggers::getLogger("trade_time_frame", true);
    #endif //TF_DEBUG

    //
    // Initialize daily time limits. Default no restrictions.
    //

    for (int i = 0; i < 7; i++)
    {
        tfr_[i] = trade_time_frame::parse_restriction("always");
    }
}

void trade_time_frame::init_from_json_object(const boost::json::object &ttf_config)
{
    using namespace boost::json;

    if (ttf_config.empty())
    {
        return;
    }

    int idx = 0;

    for (auto &day_of_week : trade_time_frame::days_)
    {
        if (ttf_config.contains(day_of_week))
        {
            value const &day_of_week_v = ttf_config.at(day_of_week);

            if (day_of_week_v.kind() != kind::string)
            {
                throw std::runtime_error("Trade time frame invalid type for restriction");
            }

            std::string restriction = day_of_week_v.get_string().c_str();

            tfr_[idx] = trade_time_frame::parse_restriction(restriction);
        }

        idx++;
    }

    #ifdef TF_DEBUG
    idx = 0;

    for (auto &day_of_week : trade_time_frame::days_)
    {
        std::string x;

        if (tfr_[idx].trade_period_from < tfr_[idx].trade_period_to)
        {
            x = boost::posix_time::to_simple_string(tfr_[idx].trade_period_from)
                + std::string(" - ")
                + boost::posix_time::to_simple_string(tfr_[idx].trade_period_to);
        }
        else
        {
            x = "never";
        }

        hft_log(INFO) << day_of_week << "[" << x << "]";

        idx++;
    }
    #endif // TF_DEBUG
}

bool trade_time_frame::can_play(const boost::posix_time::ptime &current_time_point) const
{
    boost::gregorian::date today = current_time_point.date();
    int dow = today.day_of_week();

    boost::posix_time::time_duration td = current_time_point.time_of_day();

    return trade_time_frame::can_play_today(tfr_[dow], td);
}

trade_time_frame::timeframe_restriction trade_time_frame::parse_restriction(std::string restriction)
{
    timeframe_restriction ret;

    if (restriction == "always")
    {
        ret.trade_period_from = boost::posix_time::duration_from_string("00:00:00.000");
        ret.trade_period_to   = boost::posix_time::duration_from_string("23:59:59.999");

        return ret;
    }

    if (restriction == "never")
    {
        ret.trade_period_from = boost::posix_time::duration_from_string("23:59:59.999");
        ret.trade_period_to   = boost::posix_time::duration_from_string("00:00:00.000");

        return ret;
    }

    std::vector<std::string> time_points;

    #ifdef TF_DEBUG
    hft_log(DEBUG) << "Start parsing [" << restriction << "].";
    #endif

    boost::trim_if(restriction, boost::is_any_of("\""));
    boost::split(time_points, restriction, boost::is_any_of("-"));

    if (time_points.size() != 2)
    {
        std::string error_message = std::string("Invalid time range: ")
                                    + restriction;

        #ifdef TF_DEBUG
        hft_log(ERROR) << error_message;
        #endif

        throw std::runtime_error(error_message);
    }

    ret.trade_period_from = boost::posix_time::duration_from_string(time_points.at(0));
    ret.trade_period_to   = boost::posix_time::duration_from_string(time_points.at(1));

    return ret;
}

bool trade_time_frame::can_play_today(const trade_time_frame::timeframe_restriction &restriction,
                                          const boost::posix_time::time_duration &td)
{
    return (restriction.trade_period_from <= td && restriction.trade_period_to >= td);
}
