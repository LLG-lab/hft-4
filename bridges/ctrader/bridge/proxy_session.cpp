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

#include <easylogging++.h>

#include <proxy_session.hpp>
#include <aux_functions.hpp>

#define hft2ctrader_log(__X__) \
    CLOG(__X__, "proxy")


proxy_session::proxy_session(ctrader_ssl_connection &ctrader_conn, hft_connection &hft_conn, const hft2ctrader_bridge_config &config)
    : proxy_core(ctrader_conn, hft_conn, config), config_(config)
{
    el::Loggers::getLogger("proxy", true);
}

void proxy_session::on_init(void)
{
    hft2ctrader_log(TRACE) << "on_init – got called.";

    ctrader_subscribe_instruments_ex({ "EURUSD", "GBPUSD"});
}

void proxy_session::on_tick(const tick_type &tick)
{
    static bool first_time = true;

    if (first_time)
    {
        first_time = false;

        hft2ctrader_log(TRACE) << "on_tick. instrument: ‘" << tick.instrument
                               << "’, ask: ‘" << tick.ask
                               << "’, bid: ‘" << tick.bid
                               << "’, timestamp: ‘" << tick.timestamp
                               << "’.";

        // create_market_order_ex("plandemia", tick.instrument, position_type::LONG_POSITION, 1000);
        // close_position_ex("plandemia");
    }

    hft2ctrader_log(TRACE) << "Free margin: " << get_free_margin();
}

void proxy_session::on_position_open(const position_info &position)
{
    hft2ctrader_log(INFO) << "New position OPENED:";
    hft2ctrader_log(INFO) << "    identifier:  ‘" << position.label_ << "’";
    hft2ctrader_log(INFO) << "    instrument:  " << get_instrument_ticker(position.instrument_id_);
    hft2ctrader_log(INFO) << "    opened at:   " << aux::timestamp2string(position.timestamp_);
    hft2ctrader_log(INFO) << "    volume:      " << position.volume_ << " units";
    hft2ctrader_log(INFO) << "    open price:  " << position.execution_price_;
    hft2ctrader_log(INFO) << "    used margin: " << position.used_margin_;
    hft2ctrader_log(INFO) << "    swap:        " << position.swap_;
    hft2ctrader_log(INFO) << "    commission:  " << position.commission_;

    switch (position.trade_side_)
    {
        case position_type::LONG_POSITION:
             hft2ctrader_log(INFO) << "    trade side:  LONG";
             break;
        case position_type::SHORT_POSITION:
             hft2ctrader_log(INFO) << "    trade side:  SHORT";
             break;
        case position_type::UNDEFINED_POSITION:
             // Should never happen.
             hft2ctrader_log(INFO) << "    trade side:  ???";
             break;
    }

    // TODO: Not implemented.
}

void proxy_session::on_position_open_error(const order_error_info &order_error)
{
    hft2ctrader_log(INFO) << "Open position ERROR:";
    hft2ctrader_log(INFO) << "    identifier:  ‘" << order_error.label_ << "’";
    hft2ctrader_log(INFO) << "    instrument:  " << get_instrument_ticker(order_error.instrument_id_);
    hft2ctrader_log(INFO) << "    error msg:   ‘" << order_error.error_message_ << "’";

    // TODO: Not implemented.
}

void proxy_session::on_position_close(const closed_position_info &closed_position)
{
    hft2ctrader_log(INFO) << "Position CLOSED:";
    hft2ctrader_log(INFO) << "    identifier:  ‘" << closed_position.label_ << "’";
    hft2ctrader_log(INFO) << "    instrument:  " << get_instrument_ticker(closed_position.instrument_id_);
    hft2ctrader_log(INFO) << "    close price: " << closed_position.execution_price_;

    // TODO: Not implemented.
}

void proxy_session::on_position_close_error(const order_error_info &order_error)
{
    hft2ctrader_log(INFO) << "Close position ERROR:";
    hft2ctrader_log(INFO) << "    identifier:  ‘" << order_error.label_ << "’";
    hft2ctrader_log(INFO) << "    instrument:  " << get_instrument_ticker(order_error.instrument_id_);
    hft2ctrader_log(INFO) << "    error msg:   ‘" << order_error.error_message_ << "’";

    // TODO: Not implemented.
}

void proxy_session::on_hft_advice(const hft_api::hft_response &adv)
{
    // TODO: Not implemented.
}
