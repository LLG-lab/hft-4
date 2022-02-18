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

#ifndef __BRIDGE_HPP__
#define __BRIDGE_HPP__

#include <boost/msm/back/state_machine.hpp>

#include <proxy_session.hpp>

class bridge
{
public:

    ~bridge(void) = default;

    bridge(void) = delete;

    bridge(bridge &) = delete;

    bridge(bridge &&) = delete;

    bridge(boost::asio::io_context &io_context, const hft2ctrader_config &cfg)
        : ctrader_conn_ { io_context, cfg },
          hft_conn_ { io_context, cfg },
          sm_ { ctrader_conn_, hft_conn_, cfg }
    {
        ctrader_conn_.set_on_connected_callback([this](void)
            {
                sm_.process_event(proxy_core::ctrader_connection_event());
            });

        ctrader_conn_.set_on_error_callback([this](const boost::system::error_code &ec)
            {
                ctrader_conn_.close();
                ctrader_conn_.connect();
                sm_.process_event(proxy_core::ctrader_connection_error_event(ec));
            });

        ctrader_conn_.set_on_data_callback([this](const std::vector<char> &data)
            {
                sm_.process_event(proxy_core::ctrader_data_event(data));
            });

        hft_conn_.set_on_data_callback([this](const std::string &data)
            {
                sm_.process_event(proxy_core::hft_data_event(data));
            });

        ctrader_conn_.connect();
        hft_conn_.connect();
    }

private:

    ctrader_ssl_connection ctrader_conn_;
    hft_connection hft_conn_;

    msm::back::state_machine<proxy_session> sm_;
};

#endif /* __BRIDGE_HPP__ */
