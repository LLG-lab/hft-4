/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2026 by LLG Ryszard Gradowski          **
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

#ifndef __HEARTBEAT_WATCHDOG_HPP__
#define __HEARTBEAT_WATCHDOG_HPP__

#include <boost/asio.hpp>

class heartbeat_watchdog
{
public:

    heartbeat_watchdog(void) = delete;
    heartbeat_watchdog(heartbeat_watchdog &) = delete;
    heartbeat_watchdog(heartbeat_watchdog &&) = delete;

    heartbeat_watchdog(boost::asio::io_context &io_context);

    ~heartbeat_watchdog(void) {}

    void notify(void);

private:

    void start_timer(void);

    unsigned long last_notify_time_;

    boost::asio::io_context &io_ctx_;
    boost::asio::steady_timer wakeup_timer_;
};

#endif /* __HEARTBEAT_WATCHDOG_HPP__ */

