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

#include <unistd.h>
#include <limits>

#include <hft_forex_emulator.hpp>

hft_forex_emulator::hft_forex_emulator(const std::string &host, const std::string &port, const std::string &sessid,
                                           const std::map<std::string, std::string> &instrument_data, double deposit,
                                               const std::string &config_file_name, bool check_bankruptcy,
                                                   bool invert_hft_decision, bool immediate_profit_withdrawal)
    : hft_connection_(host, port),
      balance_(deposit),
      check_bankruptcy_(check_bankruptcy),
      invert_hft_decision_(invert_hft_decision),
      immediate_profit_withdrawal_(immediate_profit_withdrawal),
      forbid_new_positions_(false),
      total_withdrawn_(0.0)
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

void hft_forex_emulator::proceed(void)
{
    tick_record tick_info;
    hft::protocol::response reply;
    double current_equity;
    double current_free_margin;
    double current_margin_level;

    int record_number = 0;
    bool is_tty_output = ::isatty(::fileno(stdout));

    while (get_record(tick_info))
    {
        while (true)
        {
            if (is_tty_output && (record_number++ % 1000 == 0))
            {
                std::cout << get_progress_str() << "\r" << std::flush;
            }

            current_equity = get_equity_at_moment();
            current_free_margin = get_free_margin_at_moment(current_equity);

            emulation_result_.total_withdrawn = total_withdrawn_;

            if (current_equity < emulation_result_.min_equity) emulation_result_.min_equity = current_equity;
            if (current_equity > emulation_result_.max_equity) emulation_result_.max_equity = current_equity;

            if (check_bankruptcy_)
            {
                if (current_equity <= 0.0)
                {
                    emulation_result_.bankrupt = true;

                    break;
                }

                current_margin_level = get_margin_level_at_moment(current_equity);

                if (current_margin_level < 1.0 && positions_.size() > 0)
                {
                    close_worst_losing_position();
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }

        if (emulation_result_.bankrupt)
        {
            break;
        }

        hft_connection_.send_tick(tick_info.instrument, balance_, current_free_margin, tick_info, reply);
        handle_response(tick_info, reply);
    }

    //
    // Data end or bankrupt. Close opened positions forcibly.
    //

    forbid_new_positions_ = true;

    while (true)
    {
        auto it = positions_.begin();

        if (it == positions_.end())
        {
            break;
        }

        tick_info = instruments_[it -> second.instrument] -> official;
        tick_info.instrument = it -> second.instrument;

        std::string pos_id = it -> first;

        handle_close_position(pos_id, tick_info, true);
    }
}

std::string hft_forex_emulator::get_progress_str(void) const
{
    std::string result;

    for (auto &item : instruments_)
    {
        result += std::to_string(item.second -> csv_faucet.get_progress());
        result += std::string("% ");
    }

    return result;
}

void hft_forex_emulator::close_worst_losing_position(void)
{
    double max_loss = std::numeric_limits<double>::max();
    std::string max_loss_pos_id;
    tick_record max_loss_tick_info;
    tick_record tick_info;

    for (auto &pos : positions_)
    {
        tick_info = instruments_[pos.second.instrument] -> official;
        tick_info.instrument = pos.second.instrument;

        auto ps = get_position_status_at_moment(pos.second, tick_info);

        if (ps.money_yield < max_loss)
        {
            max_loss_pos_id = pos.first;
            max_loss_tick_info = tick_info;
            max_loss = ps.money_yield;
        }
    }

    handle_close_position(max_loss_pos_id, max_loss_tick_info, true);
}

bool hft_forex_emulator::get_record(tick_record &tick)
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

void hft_forex_emulator::handle_response(const tick_record &tick_info, const hft::protocol::response &reply)
{
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

hft_forex_emulator::position_status hft_forex_emulator::get_position_status_at_moment(const opened_position &pos, const tick_record &tick_info)
{
    position_status ps;

    ps.moment = boost::posix_time::ptime(boost::posix_time::time_from_string(tick_info.request_time));

    int days = days_elapsed(pos.open_time, ps.moment);

    if (pos.direction == hft::protocol::response::position_direction::POSITION_LONG)
    {
        ps.total_swaps = days * (pos.qty) * instruments_[pos.instrument] -> property.get_long_dayswap_per_lot();
        ps.pips_yield = floating2pips(pos.instrument, tick_info.bid) - (pos.open_price_pips);
    }
    else if (pos.direction == hft::protocol::response::position_direction::POSITION_SHORT)
    {
        ps.total_swaps = days * (pos.qty) * instruments_[pos.instrument] -> property.get_short_dayswap_per_lot();
        ps.pips_yield = (pos.open_price_pips) - floating2pips(pos.instrument, tick_info.ask);
    }
    else
    {
        throw std::runtime_error("Illegal position direction");
    }

    ps.money_yield = 0.0;
    ps.money_yield += (ps.pips_yield) * (pos.qty) * instruments_[pos.instrument] -> property.get_pip_value_per_lot();
    ps.money_yield += ps.total_swaps;
    ps.money_yield -= 2.0 * (pos.qty) * instruments_[pos.instrument] -> property.get_commision_per_lot();

    return ps;
}

void hft_forex_emulator::handle_close_position(const std::string &id, const tick_record &tick_info, bool is_forcibly)
{
    auto it = positions_.find(id);

    if (it == positions_.end())
    {
        std::string err = std::string("HFT requested to close unopened position ‘")
                          + id + std::string("’");

        throw std::runtime_error(err);
    }

    auto ps = get_position_status_at_moment(it -> second, tick_info);

    asacp pos;
    pos.instrument      = it -> second.instrument;
    pos.direction       = it -> second.direction;
    pos.closed_forcibly = is_forcibly;
    pos.qty             = it -> second.qty;
    pos.open_time       = it -> second.open_time;
    pos.close_time      = ps.moment;

    double price = 0.0;

    if (it -> second.direction == hft::protocol::response::position_direction::POSITION_LONG)
    {
        price = tick_info.bid;
    }
    else if (it -> second.direction == hft::protocol::response::position_direction::POSITION_SHORT)
    {
        price = tick_info.ask;
    }
    else
    {
        throw std::runtime_error("Illegal position direction");
    }

    pos.total_swaps = ps.total_swaps;
    pos.pips_yield  = ps.pips_yield;
    pos.money_yield = ps.money_yield;

    if (immediate_profit_withdrawal_)
    {
        total_withdrawn_ += pos.money_yield;
    }
    else
    {
        balance_ += pos.money_yield;
    }

    positions_.erase(it);

    pos.equity = get_equity_at_moment();
    pos.used_margin_percentage = get_used_margin_percentage_at_moment(pos.equity);
    pos.still_opened = positions_.size();

    emulation_result_.trades.push_back(pos);

    hft::protocol::response reply;

    hft_connection_.send_close_notify(pos.instrument, id, true, price, reply);
    handle_response(tick_info, reply);
}

void hft_forex_emulator::handle_open_position(const hft::protocol::response::open_position_info &opi,
                                                      const tick_record &tick_info)
{
    auto it = positions_.find(opi.id_);

    if (it != positions_.end())
    {
        std::string err = std::string("HFT requested to open position ‘")
                          + opi.id_ + std::string("’ - ID already used");

        throw std::runtime_error(err);
    }

    if (forbid_new_positions_)
    {
        hft::protocol::response reply;

        hft_connection_.send_open_notify(tick_info.instrument, opi.id_, false, 0.0, reply);

        //
        // Ignore reply.
        //

        return;
    }

    double free_margin = get_free_margin_at_moment(get_equity_at_moment());

    if (free_margin < instruments_[tick_info.instrument] -> property.get_margin_required_per_lot() * opi.qty_)
    {
        hft::protocol::response reply;

        hft_connection_.send_open_notify(tick_info.instrument, opi.id_, false, 0.0, reply);
        handle_response(tick_info, reply);

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
    handle_response(tick_info, reply);
}

double hft_forex_emulator::get_equity_at_moment(void) const
{
    double equity = balance_;

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
            total_swaps = days * (qty) * instruments_.at(pos.second.instrument) -> property.get_long_dayswap_per_lot();
            pips_yield = floating2pips(pos.second.instrument, instruments_.at(pos.second.instrument) -> official.bid) - (pos.second.open_price_pips);
        }
        else if (pos.second.direction == hft::protocol::response::position_direction::POSITION_SHORT)
        {
            total_swaps = days * (qty) * instruments_.at(pos.second.instrument) -> property.get_short_dayswap_per_lot();
            pips_yield = (pos.second.open_price_pips) - floating2pips(pos.second.instrument, instruments_.at(pos.second.instrument) -> official.ask);
        }
        else
        {
            throw std::runtime_error("Illegal position direction");
        }

        equity += (pips_yield) * (qty) * instruments_.at(pos.second.instrument) -> property.get_pip_value_per_lot();
        equity += total_swaps;
        equity -= 2.0 * (qty) * instruments_.at(pos.second.instrument) -> property.get_commision_per_lot();
    }

    return equity;
}

double hft_forex_emulator::get_security_deposit(void) const
{
    double secdepo = 0.0;

    for (auto &pos : positions_)
    {
        double qty = pos.second.qty;
        secdepo += instruments_.at(pos.second.instrument) -> property.get_margin_required_per_lot() * qty;
    }

    return secdepo;
}

int hft_forex_emulator::get_used_margin_percentage_at_moment(double equity_at_moment) const
{
    if (balance_ == 0.0)
    {
        return 0.0;
    }

    auto used_margin = balance_ - get_free_margin_at_moment(equity_at_moment);

    return 100.0 * (used_margin / balance_);
}

double hft_forex_emulator::get_free_margin_at_moment(double equity_at_moment) const
{
    return equity_at_moment - get_security_deposit();
}

double hft_forex_emulator::get_margin_level_at_moment(double equity_at_moment) const
{
    if (positions_.size() == 0)
    {
        return std::numeric_limits<double>::max();
    }

    //
    // Assume that get_margin_required_per_lot() are always
    // greater than zero for all instruments.
    //

    return equity_at_moment / get_security_deposit();
}
