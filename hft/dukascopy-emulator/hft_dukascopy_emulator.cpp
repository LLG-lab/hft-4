/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                                                                    **
**          Copyright  2017 - 2021 by LLG Ryszard Gradowski          **
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

#include <hft_dukascopy_emulator.hpp>

void hft_dukascopy_emulator::proceed(void)
{
    csv_data_supplier::csv_record tick_info;
    hft::protocol::response       reply;
    double current_equity;

    while (csv_faucet_.get_record(tick_info))
    {
        current_equity = get_equity_at_moment(tick_info);

        if (current_equity < min_equity_) min_equity_ = current_equity;
        if (current_equity > max_equity_) max_equity_ = current_equity;

        if (check_bankruptcy_)
        {
            if (current_equity <= 0.0)
            {
                break;
            }
        }

        hft_connection_.send_tick(current_equity, tick_info, reply);

        if (reply.is_error())
        {
            throw std::runtime_error(reply.get_error_message());
        }

        for (auto &x : reply.get_close_positions())
        {
            handle_close_position(x, tick_info);
        }

        for (auto &x : reply.get_new_positions())
        {
            handle_open_position(x, tick_info);
        }
    }

    //
    // Data end. Close opened positions forcibly.
    //

    std::vector<std::string> ids;

    for (auto &x : positions_)
    {
        ids.push_back(x.first);
    }

    for (auto &x : ids)
    {
        handle_close_position(x, tick_info, true);
    }
}

void hft_dukascopy_emulator::handle_close_position(const std::string &id, const csv_data_supplier::csv_record &tick_info, bool is_forcibly)
{
    auto it = positions_.find(id);

    if (it == positions_.end())
    {
        std::string err = std::string("HFT requested to close unopened position ‘")
                          + id + std::string("’");

        throw std::runtime_error(err);
    }

    asacp pos;
    pos.direction       = it -> second.direction;
    pos.closed_forcibly = is_forcibly;
    pos.qty             = it -> second.qty;
    pos.open_time       = it -> second.open_time;
    pos.close_time      = boost::posix_time::ptime(boost::posix_time::time_from_string(tick_info.request_time));

    int days = days_elapsed(pos.open_time, pos.close_time);
    double total_swaps = 0.0;
    double price = 0.0;

    if (it -> second.direction == hft::protocol::response::position_direction::POSITION_LONG)
    {
        total_swaps = days * (pos.qty) * instrument_property_.get_long_dayswap_per_contract();
        pos.pips_yield = floating2pips(tick_info.bid) - (it -> second.open_price_pips);
        price = tick_info.bid;
    }
    else if (it -> second.direction == hft::protocol::response::position_direction::POSITION_SHORT)
    {
        total_swaps = days * (pos.qty) * instrument_property_.get_short_dayswap_per_contract();
        pos.pips_yield = (it -> second.open_price_pips) - floating2pips(tick_info.ask);
        price = tick_info.ask;
    }
    else
    {
        throw std::runtime_error("Illegal position direction");
    }

    pos.total_swaps = total_swaps;

    equity_ += (pos.pips_yield) * (pos.qty) * instrument_property_.get_pip_value_per_contract();
    equity_ += total_swaps;
    equity_ -= 2.0 * (pos.qty) * instrument_property_.get_commision_per_contract();

    positions_.erase(it);

    pos.equity = get_equity_at_moment(tick_info); //equity_;
    pos.still_opened = positions_.size();

    emulation_result_.push_back(pos);

    if (! is_forcibly)
    {
        hft::protocol::response reply;

        hft_connection_.send_close_notify(id, true, price, reply);
    }
}

void hft_dukascopy_emulator::handle_open_position(const hft::protocol::response::open_position_info &opi,
                                                      const csv_data_supplier::csv_record &tick_info)
{
    auto it = positions_.find(opi.id_);

    if (it != positions_.end())
    {
        std::string err = std::string("HFT requested to open position ‘")
                          + opi.id_ + std::string("’ - ID already used");

        throw std::runtime_error(err);
    }

    double price = 0.0;

    opened_position op;
    op.direction = opi.pd_;
    op.id        = opi.id_;
    op.qty       = opi.qty_;
    op.open_time = boost::posix_time::ptime(boost::posix_time::time_from_string(tick_info.request_time));

    if (op.direction == hft::protocol::response::position_direction::POSITION_LONG)
    {
        op.open_price_pips = floating2pips(tick_info.ask);
        price = tick_info.ask;
    }
    else if (op.direction == hft::protocol::response::position_direction::POSITION_SHORT)
    {
        op.open_price_pips = floating2pips(tick_info.bid);
        price = tick_info.bid;
    }
    else
    {
        throw std::runtime_error("Illegal position direction");
    }

    positions_[op.id] = op;

    hft::protocol::response reply;

    hft_connection_.send_open_notify(op.id, true, price, reply);
}

double hft_dukascopy_emulator::get_equity_at_moment(const csv_data_supplier::csv_record &tick_info) const
{
    double equity = equity_;

    for (auto &pos : positions_)
    {
        auto open_time = pos.second.open_time;
        auto now_time  = boost::posix_time::ptime(boost::posix_time::time_from_string(tick_info.request_time));

        int days = days_elapsed(open_time, now_time);

        int qty = pos.second.qty;

        double total_swaps = 0.0;
        int pips_yield = 0;

        if (pos.second.direction == hft::protocol::response::position_direction::POSITION_LONG)
        {
            total_swaps = days * (qty) * instrument_property_.get_long_dayswap_per_contract();
            pips_yield = floating2pips(tick_info.bid) - (pos.second.open_price_pips);
        }
        else if (pos.second.direction == hft::protocol::response::position_direction::POSITION_SHORT)
        {
            total_swaps = days * (qty) * instrument_property_.get_short_dayswap_per_contract();
            pips_yield = (pos.second.open_price_pips) - floating2pips(tick_info.ask);
        }
        else
        {
            throw std::runtime_error("Illegal position direction");
        }

        equity += (pips_yield) * (qty) * instrument_property_.get_pip_value_per_contract();
        equity += total_swaps;
        equity -= 2.0 * (qty) * instrument_property_.get_commision_per_contract();
    }

    return equity;
}
