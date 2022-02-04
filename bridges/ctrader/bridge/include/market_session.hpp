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

#ifndef __MARKET_SESSION_HPP__
#define __MARKET_SESSION_HPP__

#include <ctrader_session.hpp>

class market_session : public ctrader_session
{
public:

    market_session(void) = delete;
    market_session(market_session &) = delete;
    market_session(market_session &&) = delete;
    market_session(ctrader_ssl_connection &connection, const hft2ctrader_bridge_config &config);

protected:

    virtual void on_init(void) override;
    virtual void on_tick(const tick_type &tick) override;
    virtual void on_position_open(const position_info &position) override;
    virtual void on_position_open_error(const order_error_info &order_error) override;
    virtual void on_position_close(const closed_position_info &closed_position) override;
    virtual void on_position_close_error(const order_error_info &order_error) override;

private:

    const hft2ctrader_bridge_config &config_;
};

#endif /* __MARKET_SESSION_HPP__ */
