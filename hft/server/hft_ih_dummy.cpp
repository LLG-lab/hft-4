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

#include <hft_ih_dummy.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, get_logger_id().c_str())


hft_ih_dummy::hft_ih_dummy(const instrument_handler::init_info &general_config)
    : instrument_handler(general_config)
{
    el::Loggers::getLogger(get_logger_id().c_str(), true);

    hft_log(INFO) << "Starting instrument handler ’Dummy’ for instrument ‘"
                  << get_ticker() << "’, i.e. ‘"
                  << get_instrument_description()
                  << "’";
}

void hft_ih_dummy::init_handler(const boost::json::object &specific_config)
{
    hft_log(INFO) << "init_handler() Got called";
}

void hft_ih_dummy::on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market)
{
    hft_log(INFO) << "on_sync(): id ‘" << msg.id << "’, direction ‘"
                  << (msg.is_long ? "LONG" : "SHORT") << "’, open price ‘"
                  << msg.price << "’, quantity ‘" << msg.qty << "’";
}

void hft_ih_dummy::on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market)
{
    hft_log(INFO) << "on_tick(): ask ‘" << msg.ask << "’, bid ‘"
                  << msg.bid << "’, equity ‘" << msg.equity << "’";
}

void hft_ih_dummy::on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market)
{
    //
    // It will never be called.
    //
}

void hft_ih_dummy::on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market)
{
    //
    // It will never be called.
    //
}
