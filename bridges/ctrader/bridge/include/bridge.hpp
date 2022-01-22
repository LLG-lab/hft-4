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

#include <market_session.hpp>

class bridge
{
public:

    ~bridge(void) = default;

    bridge(void) = delete;

    bridge(bridge &) = delete;

    bridge(bridge &&) = delete;

    bridge(boost::asio::io_context &io_context, const hft2ctrader_bridge_config &cfg)
        : connection_ { io_context, cfg },
          sm_ { connection_, cfg }
    {
        connection_.set_on_connected_callback([this](void)
            {
                sm_.process_event(ctrader_session::new_connection_event());
            });

        connection_.set_on_error_callback([this](const boost::system::error_code &ec)
            {
                connection_.close();
                connection_.connect();
                sm_.process_event(ctrader_session::error_event(ec));
            });

        connection_.set_on_data_callback([this](const std::vector<char> &data)
            {
                sm_.process_event(ctrader_session::data_event(data));
            });

        connection_.connect();
    }

private:

    ctrader_ssl_connection connection_;

    msm::back::state_machine<market_session> sm_;
};

#endif /* __BRIDGE_HPP__ */
