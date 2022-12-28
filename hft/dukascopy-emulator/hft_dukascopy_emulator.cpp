/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System ® ≣≡=-              **
**                                                                    **
**          Copyright © 2017 - 2023 by LLG Ryszard Gradowski          **
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

#include <hft_dukascopy_emulator.hpp>

hft_dukascopy_emulator::hft_dukascopy_emulator(const std::string &host, const std::string &port, const std::string &sessid,
                                                   const std::map<std::string, std::string> &instrument_data, double deposit,
                                                       const std::string &config_file_name, bool check_bankruptcy, bool invert_hft_decision)
    : hft_connection_(host, port),
      equity_(deposit),
      check_bankruptcy_(check_bankruptcy),
      invert_hft_decision_(invert_hft_decision)
{
    std::vector<std::string> instruments;

    for (auto &instr : instrument_data)
    {
        instruments.push_back(instr.first);
        instruments_[instr.first] = std::make_shared<instrument_data_info>(instr.first, instr.second, config_file_name);
    }

    hft_connection_.init(sessid, instruments);

    proceed();
}

void hft_dukascopy_emulator::proceed(void)
{
    tick_record tick_info;
    hft::protocol::response reply;
    double current_equity;
    double current_free_margin;

    while (get_record(tick_info))
    {
        current_equity = get_equity_at_moment();
        current_free_margin = get_free_margin_at_moment(current_equity);

        if (current_equity < emulation_result_.min_equity) emulation_result_.min_equity = current_equity;
        if (current_equity > emulation_result_.max_equity) emulation_result_.max_equity = current_equity;

        if (check_bankruptcy_)
        {
            if (current_equity <= 0.0)
            {
                emulation_result_.bankrupt = true;

                break;
            }
        }

        hft_connection_.send_tick(tick_info.instrument, current_equity, current_free_margin, tick_info, reply);

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


    while (true)
    {
        auto it = positions_.begin();

        if (it == positions_.end())
        {
            break;
        }

        tick_info = instruments_[it -> second.instrument] -> official;
        tick_info.instrument = it -> second.instrument;

        handle_close_position(it -> first, tick_info, true);
    }
}

bool hft_dukascopy_emulator::get_record(tick_record &tick)
{
    for (auto &item : instruments_)
    {
        if (item.second -> state == instrument_data_info::data_state::DS_EMPTY)
        {
            bool load_result = item.second -> csv_faucet.get_record(item.second -> loaded);

            if (load_result)
            {
                item.second -> state = instrument_data_info::data_state::DS_LOADED;
            }
            else
            {
                item.second -> state = instrument_data_info::data_state::DS_EOF;
            }
        }
    }

    std::string   earliest_instrument;
    unsigned long earliest_time = std::numeric_limits<unsigned long>::max();

    for (auto &item : instruments_)
    {
        if (item.second -> state == instrument_data_info::data_state::DS_LOADED)
        {
            auto t = hft::utils::ptime2timestamp(boost::posix_time::ptime(boost::posix_time::time_from_string(item.second -> loaded.request_time)));

            if (t < earliest_time)
            {
                earliest_time = t;
                earliest_instrument = item.first;
            }
        }
    }

    if (earliest_instrument.length() > 0)
    {
        instruments_[earliest_instrument] -> official = instruments_[earliest_instrument] -> loaded;
        instruments_[earliest_instrument] -> state = instrument_data_info::data_state::DS_EMPTY;

        tick = instruments_[earliest_instrument] -> official;
        tick.instrument = earliest_instrument;

        return true;
    }

    return false;
}

void hft_dukascopy_emulator::handle_close_position(const std::string &id, const tick_record &tick_info, bool is_forcibly)
{
    auto it = positions_.find(id);

    if (it == positions_.end())
    {
        std::string err = std::string("HFT requested to close unopened position ‘")
                          + id + std::string("’");

        throw std::runtime_error(err);
    }

    asacp pos;
    pos.instrument      = it -> second.instrument;
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
        total_swaps = days * (pos.qty) * instruments_[pos.instrument] -> property.get_long_dayswap_per_contract();
        pos.pips_yield = floating2pips(pos.instrument, tick_info.bid) - (it -> second.open_price_pips);
        price = tick_info.bid;
    }
    else if (it -> second.direction == hft::protocol::response::position_direction::POSITION_SHORT)
    {
        total_swaps = days * (pos.qty) * instruments_[pos.instrument] -> property.get_short_dayswap_per_contract();
        pos.pips_yield = (it -> second.open_price_pips) - floating2pips(pos.instrument, tick_info.ask);
        price = tick_info.ask;
    }
    else
    {
        throw std::runtime_error("Illegal position direction");
    }

    pos.total_swaps = total_swaps;

    equity_ += (pos.pips_yield) * (pos.qty) * instruments_[pos.instrument] -> property.get_pip_value_per_contract();
    equity_ += total_swaps;
    equity_ -= 2.0 * (pos.qty) * instruments_[pos.instrument] -> property.get_commision_per_contract();

    positions_.erase(it);

    pos.equity = get_equity_at_moment(); //equity_;
    pos.still_opened = positions_.size();

    emulation_result_.trades.push_back(pos);

    if (! is_forcibly)
    {
        hft::protocol::response reply;

        hft_connection_.send_close_notify(pos.instrument, id, true, price, reply);
    }
}

void hft_dukascopy_emulator::handle_open_position(const hft::protocol::response::open_position_info &opi,
                                                      const tick_record &tick_info)
{
    auto it = positions_.find(opi.id_);

    if (it != positions_.end())
    {
        std::string err = std::string("HFT requested to open position ‘")
                          + opi.id_ + std::string("’ - ID already used");

        throw std::runtime_error(err);
    }

    double free_margin = get_free_margin_at_moment(get_equity_at_moment());

    if (free_margin < instruments_[tick_info.instrument] -> property.get_margin_required_per_contract() * opi.qty_)
    {
        hft::protocol::response reply;

        hft_connection_.send_open_notify(tick_info.instrument, opi.id_, false, 0.0, reply);

        return;
    }

    double price = 0.0;

    opened_position op;
    op.instrument = tick_info.instrument;
    op.id         = opi.id_;
    op.qty        = opi.qty_;
    op.open_time  = boost::posix_time::ptime(boost::posix_time::time_from_string(tick_info.request_time));

    if (invert_hft_decision_)
    {
        if (opi.pd_ == hft::protocol::response::position_direction::POSITION_LONG)
        {
            op.direction = hft::protocol::response::position_direction::POSITION_SHORT;
        }
        else if (opi.pd_ == hft::protocol::response::position_direction::POSITION_SHORT)
        {
            op.direction = hft::protocol::response::position_direction::POSITION_LONG;
        }
    }
    else
    {
        op.direction  = opi.pd_;
    }


    if (op.direction == hft::protocol::response::position_direction::POSITION_LONG)
    {
        op.open_price_pips = floating2pips(op.instrument, tick_info.ask);
        price = invert_hft_decision_ ? tick_info.bid : tick_info.ask;
    }
    else if (op.direction == hft::protocol::response::position_direction::POSITION_SHORT)
    {
        op.open_price_pips = floating2pips(op.instrument, tick_info.bid);
        price = invert_hft_decision_ ? tick_info.ask : tick_info.bid;
    }
    else
    {
        throw std::runtime_error("Illegal position direction");
    }

    positions_[op.id] = op;

    hft::protocol::response reply;

    hft_connection_.send_open_notify(op.instrument, op.id, true, price, reply);
}

double hft_dukascopy_emulator::get_equity_at_moment(void) const
{
    double equity = equity_;

    for (auto &pos : positions_)
    {
        auto open_time = pos.second.open_time;
        auto now_time  = boost::posix_time::ptime(boost::posix_time::time_from_string(instruments_.at(pos.second.instrument) -> official.request_time));

        int days = days_elapsed(open_time, now_time);

        double qty = pos.second.qty;

        double total_swaps = 0.0;
        int pips_yield = 0;

        if (pos.second.direction == hft::protocol::response::position_direction::POSITION_LONG)
        {
            total_swaps = days * (qty) * instruments_.at(pos.second.instrument) -> property.get_long_dayswap_per_contract();
            pips_yield = floating2pips(pos.second.instrument, instruments_.at(pos.second.instrument) -> official.bid) - (pos.second.open_price_pips);
        }
        else if (pos.second.direction == hft::protocol::response::position_direction::POSITION_SHORT)
        {
            total_swaps = days * (qty) * instruments_.at(pos.second.instrument) -> property.get_short_dayswap_per_contract();
            pips_yield = (pos.second.open_price_pips) - floating2pips(pos.second.instrument, instruments_.at(pos.second.instrument) -> official.ask);
        }
        else
        {
            throw std::runtime_error("Illegal position direction");
        }

        equity += (pips_yield) * (qty) * instruments_.at(pos.second.instrument) -> property.get_pip_value_per_contract();
        equity += total_swaps;
        equity -= 2.0 * (qty) * instruments_.at(pos.second.instrument) -> property.get_commision_per_contract();
    }

    return equity;
}

double hft_dukascopy_emulator::get_free_margin_at_moment(double equity_at_moment) const
{
    double used_margin = 0.0;

    for (auto &pos : positions_)
    {
        double qty = pos.second.qty;
        used_margin += instruments_.at(pos.second.instrument) -> property.get_margin_required_per_contract() * qty;
    }

    return equity_at_moment - used_margin;
}
