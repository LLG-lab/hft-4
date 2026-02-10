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

#include <easylogging++.h>

#include <proxy_session.hpp>
#include <aux_functions.hpp>

#define hft2ctrader_log(__X__) \
    CLOG(__X__, "proxy")


proxy_session::proxy_session(ctrader_ssl_connection &ctrader_conn, hft_connection &hft_conn, heartbeat_watchdog &hw, const hft2ctrader_config &config)
    : proxy_core(ctrader_conn, hft_conn, hw, config),
      hft_session_initialized_ {false},
      config_ {config}
{
    el::Loggers::getLogger("proxy", true);
}

void proxy_session::on_init(void)
{
    hft2ctrader_log(INFO)  << "Subscribing instruments for "
                           << get_broker();

    if (! ctrader_subscribe_instruments_ex(config_.get_instruments()))
    {
        hft2ctrader_log(FATAL) << "Unable to subscribe instruments – cannot proceed, this is fatal.";
    }

    if (! ctrader_instruments_information_ex(config_.get_instruments()))
    {
        hft2ctrader_log(FATAL) << "Unable to obtain detailed instruments information – cannot proceed, this is fatal.";
    }

    if (! hft_session_initialized_)
    {
        hft2ctrader_log(INFO)  << "Initialize HFT session, id ‘"
                               << config_.get_session_id() << "’";

        hft_init_session(config_.get_session_id(), config_.get_instruments());

        hft_session_initialized_ = true;
    }
    else
    {
        hft2ctrader_log(INFO)  << "Skipping session initialization on HFT server side"
                               << " – session already initialized.";
    }

    //
    // Synchronize HFT with already opened positions.
    //

    auto &positions = get_opened_positions();

    for (auto &position : positions)
    {
        if (position.label_.length() == 0)
        {
            hft2ctrader_log(WARNING) << "Position #" << position.position_id_
                                     << " will not be synchronized with HFT server,"
                                     << " since does not have an assigned identifier.";

            hft2ctrader_log(WARNING) << "Unsynchronized - id: #" << position.position_id_
                                     << ", instrument: ‘" << get_instrument_ticker(position.instrument_id_)
                                     << "’, volume: " << position.volume_ << " units, opened at: "
                                     << aux::timestamp2string(position.timestamp_) << ", trade side: "
                                     << position_type_str(position.trade_side_);
        }
        else
        {
            hft2ctrader_log(INFO) << "SYNCHRONIZING position with HFT server:";
            hft2ctrader_log(INFO) << "    identifier:  ‘" << position.label_ << "’";
            hft2ctrader_log(INFO) << "    instrument:  " << get_instrument_ticker(position.instrument_id_);
            hft2ctrader_log(INFO) << "    opened at:   " << aux::timestamp2string(position.timestamp_);
            hft2ctrader_log(INFO) << "    volume:      " << position.volume_ << " units";
            hft2ctrader_log(INFO) << "    open price:  " << position.execution_price_;
            hft2ctrader_log(INFO) << "    used margin: " << position.used_margin_;
            hft2ctrader_log(INFO) << "    swap:        " << position.swap_;
            hft2ctrader_log(INFO) << "    commission:  " << position.commission_;
            hft2ctrader_log(INFO) << "    trade side:  " << position_type_str(position.trade_side_);

            hft_sync(get_instrument_ticker(position.instrument_id_),
                     position.timestamp_,
                     position.label_,
                     position.trade_side_,
                     position.execution_price_,
                     position.volume_
            );
        }
    }
}

void proxy_session::on_tick(const tick_type &tick)
{
    hft2ctrader_log(TRACE) << "Proxying tick: instrument: ‘" << tick.instrument
                               << "’, ask: ‘" << tick.ask
                               << "’, bid: ‘" << tick.bid
                               << "’, timestamp: ‘" << tick.timestamp
                               << "’, free margin: " << get_free_margin();

    hft_send_tick(tick.instrument, tick.timestamp, tick.ask, tick.bid, get_balance(), get_free_margin());
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
    hft2ctrader_log(INFO) << "    trade side:  " << position_type_str(position.trade_side_);

    hft_send_open_notify(get_instrument_ticker(position.instrument_id_),
                         position.label_,
                         true,
                         position.execution_price_);
}

void proxy_session::on_position_open_error(const order_error_info &order_error)
{
    hft2ctrader_log(ERROR) << "Open position ERROR:";
    hft2ctrader_log(ERROR) << "    identifier:  ‘" << order_error.label_ << "’";
    hft2ctrader_log(ERROR) << "    instrument:  " << get_instrument_ticker(order_error.instrument_id_);
    hft2ctrader_log(ERROR) << "    error msg:   ‘" << order_error.error_message_ << "’";

    hft_send_open_notify(get_instrument_ticker(order_error.instrument_id_),
                         order_error.label_,
                         false,
                         0.0);
}

void proxy_session::on_position_close(const closed_position_info &closed_position)
{
    hft2ctrader_log(INFO) << "Position CLOSED:";
    hft2ctrader_log(INFO) << "    identifier:  ‘" << closed_position.label_ << "’";
    hft2ctrader_log(INFO) << "    instrument:  " << get_instrument_ticker(closed_position.instrument_id_);
    hft2ctrader_log(INFO) << "    close price: " << closed_position.execution_price_;

    hft_send_close_notify(get_instrument_ticker(closed_position.instrument_id_),
                          closed_position.label_,
                          true,
                          closed_position.execution_price_);
}

void proxy_session::on_position_close_error(const order_error_info &order_error)
{
    hft2ctrader_log(ERROR) << "Close position ERROR:";
    hft2ctrader_log(ERROR) << "    identifier:  ‘" << order_error.label_ << "’";
    hft2ctrader_log(ERROR) << "    instrument:  " << get_instrument_ticker(order_error.instrument_id_);
    hft2ctrader_log(ERROR) << "    error msg:   ‘" << order_error.error_message_ << "’";

    hft_send_close_notify(get_instrument_ticker(order_error.instrument_id_),
                          order_error.label_,
                          false,
                          0.0);
}

void proxy_session::on_hft_advice(const hft_api::hft_response &adv, bool broker_ready)
{
    if (adv.is_error())
    {
        hft2ctrader_log(ERROR) << "HFT ERROR: " << adv.get_error_message();

        return;
    }

    if (adv.has_for_close())
    {
        auto &to_be_closed = adv.get_for_close();

        if (! adv.has_instrument())
        {
            hft2ctrader_log(ERROR) << "PROTOCOL ERROR: Instrument missing !!!";

            return;
        }

        for (auto &identifier : to_be_closed)
        {
            hft2ctrader_log(INFO) << "HFT requested to close position ‘"
                                  << identifier << "’.";

            if (! broker_ready || ! ctrader_close_position_ex(identifier))
            {
                hft_send_close_notify(adv.get_instrument(),
                                      identifier,
                                      false,
                                      0.0);
            }
        }
    }

    if (adv.has_for_open())
    {
        auto &to_be_open = adv.get_for_open();

        if (! adv.has_instrument())
        {
            hft2ctrader_log(ERROR) << "PROTOCOL ERROR: Instrument missing !!!";

            return;
        }

        for (auto &pinfo : to_be_open)
        {
            hft2ctrader_log(INFO) << "HFT requested to open position "
                                  << position_type_str(pinfo.direction)
                                  << " identified as ‘"
                                  << pinfo.identifier << "’ for instrument ‘"
                                  << adv.get_instrument() << "’; volume: "
                                  << pinfo.volume << " units.";

            if (! broker_ready || ! ctrader_create_market_order_ex(pinfo.identifier, adv.get_instrument(), pinfo.direction, pinfo.volume))
            {
                hft_send_open_notify(adv.get_instrument(),
                                     pinfo.identifier,
                                     false,
                                     0.0);

            }
        }
    }
}
