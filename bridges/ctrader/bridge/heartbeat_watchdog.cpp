/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2025 by LLG Ryszard Gradowski          **
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

#include <stdexcept>

#include <heartbeat_watchdog.hpp>
#include <aux_functions.hpp>

#include <easylogging++.h>

#define hft2ctrader_log(__X__) \
    CLOG(__X__, "heartbeat_watchdog")

heartbeat_watchdog::heartbeat_watchdog(boost::asio::io_context &io_context)
    : io_ctx_ {io_context}, wakeup_timer_ {io_ctx_}
{
    el::Loggers::getLogger("heartbeat_watchdog", true);

    notify();

    start_timer();
}

void heartbeat_watchdog::notify(void)
{
    last_notify_time_ = aux::get_current_timestamp();
}

void heartbeat_watchdog::start_timer(void)
{
    wakeup_timer_.expires_after(boost::asio::chrono::seconds(120));
    wakeup_timer_.async_wait(
        [this](const boost::system::error_code &error)
        {
            if (! error)
            {
                hft2ctrader_log(TRACE) << "Check";

                auto now = aux::get_current_timestamp();
                auto diff_sec = (now - last_notify_time_) / 1000;

                if (diff_sec > 120)
                {
                    hft2ctrader_log(ERROR) << "No heart beat detected within defined time – 120 seconds.";

                    throw std::runtime_error("Heartbeat notify timeout");
                }

                start_timer();
            }
            else if (error == boost::asio::error::operation_aborted)
            {
                hft2ctrader_log(WARNING) << "Timer aborted!";
            }
        }
    );
}
