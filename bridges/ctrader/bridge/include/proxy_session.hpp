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

#ifndef __PROXY_SESSION_HPP__
#define __PROXY_SESSION_HPP__

#include <proxy_core.hpp>

class proxy_session : public proxy_core
{
public:

    proxy_session(void) = delete;
    proxy_session(proxy_session &) = delete;
    proxy_session(proxy_session &&) = delete;
    proxy_session(ctrader_ssl_connection &ctrader_conn, hft_connection &hft_conn, heartbeat_watchdog &hw, const hft2ctrader_config &config);

protected:

    virtual void on_init(void) override;
    virtual void on_tick(const tick_type &tick) override;
    virtual void on_position_open(const position_info &position) override;
    virtual void on_position_open_error(const order_error_info &order_error) override;
    virtual void on_position_close(const closed_position_info &closed_position) override;
    virtual void on_position_close_error(const order_error_info &order_error) override;
    virtual void on_hft_advice(const hft_api::hft_response &adv, bool broker_ready) override;

private:

    bool hft_session_initialized_;

    const hft2ctrader_config &config_;
};

#endif /* __PROXY_SESSION_HPP__ */
