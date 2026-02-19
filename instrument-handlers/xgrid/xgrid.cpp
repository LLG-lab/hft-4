#include <xgrid.hpp>
#include <utilities.hpp>

#include <limits>
#include <cctype>

#include <boost/lexical_cast.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, get_logger_id().c_str())

namespace hft_ih_plugin
{

xgrid::xgrid(const instrument_handler::init_info &general_config)
    : instrument_handler(general_config),
      current_state_ {state::OPERATIONAL},
      max_spread_ {0xFFFF},
      active_gcells_limit_ {0},
      sellout_ {false},
      immediate_money_supply_ {0.0},
      pip_value_per_lot_ {0.0},
      margin_required_per_lot_ {0.0},
      commission_per_lot_ {0.0},
      long_dayswap_per_lot_ {0.0},
      used_cells_alarm_ {0},
      user_alarmed_ {false},
      positions_confirmed_ {false},
      awaiting_position_status_counter_ {0},
      opened_positions_counter_ {0},
      tick_counter_ {0ul}
{
    el::Loggers::getLogger(get_logger_id().c_str(), true);

    hft_log(INFO) << "Starting instrument handler ‘eXtended Grid™ version 2’ for instrument ‘"
                  << get_ticker() << "’, i.e. ‘"
                  << get_instrument_description()
                  << "’";
}

void xgrid::init_handler(const boost::json::object &specific_config)
{
    //
    // Going to obtain following attributes (example):
    //

    //
    //  "handler_options": {
    //      "transactions": {
    //          .
    //          .
    //          .
    //      },
    //      "instrument_details": {
    //          .
    //          .
    //          .
    //      },
    //      "max_spread": 12,
    //      "active_gcells_limit":10, /* Optional, default: gcells */
    //      "used_cells_alarm":20,    /* Optional, default: 0 (disabled) */
    //      "sellout":false,          /* Optional, default: false */
    //      "immediate_money_supply":0.0,
    //      "grid_definition": {
    //           .
    //           .
    //           .
    //      },
    //      "invest_guard": {         /* Optional */
    //          "enabled": true,
    //          "alpha": 0.34,
    //          "beta": 0.99879
    //      }
    //  }
    //

    try
    {
        //
        // Money management stuff.
        //

        const boost::json::object &transactions = json_get_object_attribute(specific_config, "transactions");

        create_money_manager(transactions);

        //
        // Parse instrument details.
        //

        const boost::json::object &instrument_details = json_get_object_attribute(specific_config, "instrument_details");

        parse_instrument_details(instrument_details);

        max_spread_ = json_get_int_attribute(specific_config, "max_spread");

        if (max_spread_ <= 0)
        {
            std::string msg = "Attribute ‘max_spread’ must be greater than 0, got "
                              + std::to_string(max_spread_);

            throw std::runtime_error(msg.c_str());
        }

        if (json_exist_attribute(specific_config, "active_gcells_limit"))
        {
            active_gcells_limit_ = json_get_int_attribute(specific_config, "active_gcells_limit");

            if (active_gcells_limit_ <= 0)
            {
                std::string msg = "Attribute ‘active_gcells_limit’ must be greater than 0, got "
                                  + std::to_string(active_gcells_limit_);

                throw std::runtime_error(msg.c_str());
            }

            hft_log(INFO) << "init: Active gcells are limited to "
                          << active_gcells_limit_ << ".";
        }
        else
        {
            hft_log(INFO) << "init: Active gcells are unlimited.";

            active_gcells_limit_ = std::numeric_limits<int>::max();
        }

        if (json_exist_attribute(specific_config, "used_cells_alarm"))
        {
            used_cells_alarm_ = json_get_int_attribute(specific_config, "used_cells_alarm");

            if (used_cells_alarm_ < 0)
            {
                std::string msg = "Attribute ‘used_cells_alarm’ must be ≥ 0, got "
                                  + std::to_string(used_cells_alarm_);

                throw std::runtime_error(msg.c_str());
            }

            hft_log(INFO) << "init: User will be notified when the "
                          << used_cells_alarm_ << "th position is opened.";
        }

        if (json_exist_attribute(specific_config, "sellout"))
        {
            sellout_ = json_get_bool_attribute(specific_config, "sellout");

            if (sellout_)
            {
                hft_log(INFO) << "init: SELLOUT mode enabled, no new "
                              << "positions will be opened.";
            }
        }

        if (json_exist_attribute(specific_config, "immediate_money_supply"))
        {
            immediate_money_supply_ = json_get_double_attribute(specific_config, "immediate_money_supply");

            if (immediate_money_supply_ < 0.0)
            {
                std::string msg = "Attribute ‘immediate_money_supply’ must be ≥ 0, got "
                                  + std::to_string(immediate_money_supply_);

                throw std::runtime_error(msg.c_str());
            }

            hft_log(INFO) << "init: IMS is " << immediate_money_supply_;
        }

        const boost::json::object &grid_def = json_get_object_attribute(specific_config, "grid_definition");

        create_grid(grid_def);

        hft_log(INFO) << "init: Created grid with ‘"
                      << gcells_.size() << "’.";

        for (auto &x : gcells_)
        {
            hft_log(INFO) << "\t#" << x.get_id() << " – ["
                          << pips2floating(x.get_min_limit())
                          << " ... "
                          << pips2floating(x.get_max_limit())
                          << "]" << (x.is_terminal() ? " (terminal)" : "");
        }

        int range_min, range_max;

        for (auto it = gcells_.begin(); it != gcells_.end(); it++)
        {
            if (! it -> is_terminal())
            {
                hft_log(INFO) << "init: Trades from " << pips2floating(it -> get_min_limit());

                range_min = it -> get_min_limit();

                break;
            }
        }

        for (auto it = gcells_.rbegin(); it != gcells_.rend(); it++)
        {
            if (! it -> is_terminal())
            {
                hft_log(INFO) << "init: Trades to " << pips2floating(it -> get_max_limit());

                range_max = it -> get_max_limit();

                break;
            }
        }

        int max_optimistic_loss = 0;
        int max_pessimistic_loss = 0;

        for (auto &x : gcells_)
        {
            if (x.is_terminal())
            {
                continue;
            }

            max_optimistic_loss += (x.get_max_limit() - gcells_[0].get_min_limit());
            max_pessimistic_loss += x.get_max_limit();
        }

        hft_log(INFO) << "init: Estimation of maximum losses for grid architecture: "
                      << "pessimistic – " << max_pessimistic_loss
                      << "pips, optimistic – " << max_optimistic_loss
                      << "pips.";

        load_positions();

        //
        // Invest guard configuration.
        //

        if (json_exist_attribute(specific_config, "invest_guard"))
        {
            const boost::json::object &invest_guard_json = json_get_object_attribute(specific_config, "invest_guard");

            bool enabled = json_get_bool_attribute(invest_guard_json, "enabled");
            double alpha = json_get_double_attribute(invest_guard_json, "alpha");
            double beta  = json_get_double_attribute(invest_guard_json, "beta");

            if (enabled)
            {
                iguard_.enable();
            }

            iguard_.setup_range(range_min, range_max);
            iguard_.set_alpha(alpha);
            iguard_.set_beta(beta);
            iguard_.set_pain_svr(session_variable("xgrid.invest_guard.pain"));
        }
    }
    catch (const std::runtime_error &e)
    {
        std::string err_message = std::string(e.what())
                                  + std::string(" in handler_options of ")
                                  + get_ticker()
                                  + std::string(" instrument manifest");

        hft_log(ERROR) << "init: " << err_message;

        throw std::runtime_error(err_message);
    }
}

void xgrid::on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market)
{
    for (position_container::iterator it = positions_.begin(); it != positions_.end(); ++it)
    {
        if (it -> position_id_ == msg.id)
        {
            it -> position_confirmed_ = true;

            return;
        }
    }

    hft_log(WARNING) << "sync: Unrecognized position: ‘"
                     << msg.id << "’.";
}

void xgrid::on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market)
{
    verify_position_confirmation_status();

    int ask_pips = floating2pips(msg.ask);
    int bid_pips = floating2pips(msg.bid);
    int spread   = ask_pips - bid_pips;
    unsigned long request_timestamp = hft::utils::ptime2timestamp(msg.request_time);

    if ((tick_counter_++ % 60) == 0)
    {
        update_metrics(bid_pips, msg.equity, msg.request_time);
    }

    if (current_state_ == state::WAIT_FOR_STATUS)
    {
        await_position_status();

        return;
    }

    if (spread > max_spread_)
    {
        return;
    }

    iguard_.tick(ask_pips, request_timestamp);

    int index = -1;

    for (int i = 0; i < gcells_.size(); i++)
    {
        if (gcells_[i].inside_trading_zone(ask_pips))
        {
            index = i;
            break;
        }
    }

    if (index == -1)
    {
        return;
    }

    //
    // We're in trading zone.
    //

    //
    // We have four separate conditions:
    //
    //   index |     |     |  •  |  •  |
    // --------+-----+-----+-----+-----+
    // index–n |     |  •  |     |  •  |
    //           (I)  (II)  (III) (IV)

    // Conditions (I) / (II).
    if (! gcells_[index].has_position() && ! gcells_[index].is_terminal())
    {
        int precedessor_index = get_precedessor_position_index(index);

        if (precedessor_index < 0) // Conditions (I).
        {
            if (opened_positions_counter_ < active_gcells_limit_ && !sellout_)
            {
                if (iguard_.can_play())
                {
                    double bankroll = msg.equity + immediate_money_supply_;
                    double num_of_lots = mmgmnt_ -> get_number_of_lots(bankroll);

                    if (num_of_lots > 0.0)
                    {
                        auto pos_id = uid();
                        market.open_long(pos_id, num_of_lots);
                        gcells_[index].attach_position(pos_id, num_of_lots, request_timestamp);

                        hft_log(INFO) << "Opening position ‘"
                                      << pos_id << "’ in cell #"
                                      << gcells_[index].get_id()
                                      << ", balance before open: "
                                      << msg.equity
                                      << ", whole money including IMS: "
                                      << bankroll;

                        current_state_ = state::WAIT_FOR_STATUS;
                    }
                    else
                    {
                        hft_log(INFO) << "Insufficient volume to open position on market, "
                                      << "creating Virtual Position instead.";

                        gcells_[index].attach_confirmed_virtual_position(request_timestamp, ask_pips);

                        save_positions();
                    }
                }
                else
                {
                    if ((tick_counter_ % 500) == 0)
                    {
                        hft_log(INFO) << "Refusal to open a position by Invest Guard.";
                    }
                }
            }
            else
            {
                if (!sellout_ && (tick_counter_ % 500) == 0)
                {
                    hft_log(INFO) << "Cannot open position, since amount "
                                  << "of active gcells has reached the limit ‘"
                                  << active_gcells_limit_ << "’.";
                }
            }

            return;
        }
        else
        {
            // Condition (II) – do nothing.
            // Because we have to close position from cell of precedessor_index first.
        }
    }

    // For Condition (III) and Condition (IV) do nothing.

    //
    // Attempt to liquidate pyramid of all gcells from 0 to index - 1.
    //

    for (int i = 0; i < index; i++)
    {
        if (gcells_[i].has_position() && profitable(i, bid_pips, msg.request_time))
        {
            // Close position.

            if (gcells_[i].get_position_id() == "virtual")
            {
                hft_log(INFO) << "Closing Virtual Position.";

                gcells_[i].detatch_position();

                save_positions();
            }
            else
            {
                market.close_position(gcells_[i].get_position_id());

                hft_log(INFO) << "Closing position ‘" << gcells_[i].get_position_id()
                              << "’ from cell #" << gcells_[i].get_id();

                current_state_ = state::WAIT_FOR_STATUS;

                return;
            }
        }
    }

    //
    // Check for user alarm.
    //

    if (used_cells_alarm_ > 0 && ! user_alarmed_)
    {
        if (opened_positions_counter_ >= used_cells_alarm_)
        {
            std::string message = "HFT handler msg: " + get_ticker_fmt2()
                                  + " opened " + std::to_string(opened_positions_counter_)
                                  + "th position";

            user_alarmed_ = true;
            sms_alert(message);
            save_positions();
        }
    }
}

void xgrid::on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market)
{
    if (current_state_ != state::WAIT_FOR_STATUS)
    {
        hft_log(ERROR) << "position_open: Unexpected position open notify";
    }

    auto it = positions_.begin();

    while (it != positions_.end())
    {
        if (it -> position_id_ == msg.id)
        {
            if (msg.status)
            {
                hft_log(INFO) << "position_open: Position ‘" << msg.id
                              << "’ successfuly opened, price was "
                              << msg.price;

                it -> position_price_pips_ = floating2pips(msg.price);
                it -> position_confirmed_ = true;

                save_positions();
            }
            else
            {
                hft_log(INFO) << "position_open: Failed to open position ‘"
                              << msg.id << "’.";

                if (it -> gcell_number_ >= 0)
                {
                    gcells_[it -> gcell_number_].detatch_position(it);
                }
                else
                {
                     // Sytuacja, gdy pozycja znajduje się na liście positions_
                     // Lecz nie jest zarządzana przez grid (bo na przykład
                     // architektura grida uległa zmianie i pozycja nie trafiła
                     // w przedziały), natomiast pozycja została zamknięta
                     // „z ręki”.
                     // XXX: To nigdy nie bedzie miało miejsca w przypadku otwierania.
                }
            }

            break;
        }

        ++it;
    }

    current_state_ = state::OPERATIONAL;
    awaiting_position_status_counter_ = 0;
}

void xgrid::on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market)
{
    if (current_state_ != state::WAIT_FOR_STATUS)
    {
        hft_log(ERROR) << "position_close: Unexpected position close notify";
    }

    if (msg.status)
    {
        auto it = positions_.begin();

        while (it != positions_.end())
        {
            if (it -> position_id_ == msg.id)
            {
                if (it -> gcell_number_ >= 0)
                {
                    gcells_[it -> gcell_number_].detatch_position(it);
                }
                else
                {
                    // Sytuacja, gdy pozycja znajduje się na liście positions_
                    // Lecz nie jest zarządzana przez grid (bo na przykład
                    // architektura grida uległa zmianie i pozycja nie trafiła
                    // w przedziały), natomiast pozycja została zamknięta
                    // „z ręki”.

                    positions_.erase(it);
                }

                save_positions();

                hft_log(INFO) << "position_close: Closed position ‘"
                              << msg.id << "’, price: " << msg.price;

                break;
            }

            ++it;
        }
    }
    else
    {
        hft_log(INFO) << "position_close: Failed to close position ‘"
                      << msg.id << "’.";

    }

    current_state_ = state::OPERATIONAL;
    awaiting_position_status_counter_ = 0;
}

//
// Private methods.
//

void xgrid::update_metrics(int bid_pips, double bankroll, boost::posix_time::ptime current_time)
{
    if (! metrics::is_service_enabled())
    {
        return;
    }

    double total_expense = 0.0;

    for (auto &pos : positions_)
    {
        if (pos.position_id_ != "virtual")
        {
            int days_elapsed = (current_time.date() - hft::utils::timestamp2ptime(pos.position_time_).date()).days();
            double swaps_expense = long_dayswap_per_lot_ * (pos.position_volume_) * days_elapsed;
            double yield = pip_value_per_lot_ * (pos.position_volume_) * (bid_pips - pos.position_price_pips_);
            double req_margin = margin_required_per_lot_ * (pos.position_volume_);
            double commission = commission_per_lot_ * (pos.position_volume_);

            total_expense += (yield + swaps_expense - req_margin - commission);
        }
    }

    double percentage_use_of_margin = (((-1.0)*total_expense) / bankroll) * 100;

    setup_percentage_use_of_margin_metric(percentage_use_of_margin);
    setup_opened_positions_metric(opened_positions_counter_);
}

bool xgrid::profitable(int cell_index, int bid_pips, boost::posix_time::ptime current_time)
{
    //
    // Swaps by the design are not taken into account when calculating profitability,
    // even though it may lead to closing the position at a loss.
    //
    // (XXX) int days_elapsed = (current_time.date() - hft::utils::timestamp2ptime(gcells_[cell_index].get_position_timestamp()).date()).days();
    // (XXX) int yield = bid_pips - gcells_[cell_index].get_position_price_pips() + dayswap_pips_*days_elapsed;
    //

    int yield = bid_pips - gcells_[cell_index].get_position_price_pips();

    if (yield >= gcells_[cell_index].span())
    {
        return true;
    }

    return false;
}

int xgrid::get_precedessor_position_index(int index) const
{
    for (int i = index - 1; i >= 0; i--)
    {
        if (gcells_[i].has_position())
        {
            return i;
        }
    }

    return -1;
}

std::map<char, std::pair<int, bool>> xgrid::get_cell_types(const boost::json::object &obj) const
{
    using namespace boost::json;

    std::map<char, std::pair<int, bool>> result;

    if (! obj.contains("cell_types"))
    {
        std::string error_msg = std::string("Missing attribute ‘cell_types’");

        throw std::runtime_error(error_msg);
    }

    value const &attr_v = obj.at("cell_types");

    if (attr_v.kind() != kind::array)
    {
        std::string error_msg = std::string("Invalid attribute type for ‘cell_types’ - array type expected");

        throw std::runtime_error(error_msg);
    }

    array const &arr = attr_v.get_array();

    for (auto it = arr.begin(); it != arr.end(); it++)
    {
        value const &array_val = *it;

        if (array_val.kind() != kind::object)
        {
            std::string error_msg = std::string("Invalid item type of array attribute ‘cell_types’ - object type expected");

            throw std::runtime_error(error_msg);

        }
        else
        {
            const boost::json::object &cell_type_obj = array_val.get_object();

            std::string id = json_get_string_attribute(cell_type_obj, "class");
            int pips_span  = json_get_int_attribute(cell_type_obj, "pips_span");
            bool terminal  = false;

            if (json_exist_attribute(cell_type_obj, "terminal"))
            {
                terminal = json_get_bool_attribute(cell_type_obj, "terminal");
            }

            if (id.length() != 1 || std::isspace(static_cast<unsigned char>(id[0])))
            {
                std::string error_msg = std::string("Invalid class ID in cell type definition: ‘") + id
                                        + std::string("’. Class ID must be single non-whitespace character");

                throw std::runtime_error(error_msg);
            }

            if (pips_span <= 0)
            {
                std::string error_msg = std::string("Invalid pips span in cell type definition: ‘")
                                        + std::to_string(pips_span) + std::string("’. Pips span must be positive value");

                throw std::runtime_error(error_msg);
            }

            if (result.count(id[0]) > 0)
            {
                std::string error_msg = std::string("Duplicate definition class: ‘") + id
                                        + std::string("’ in cell type definition.");

                throw std::runtime_error(error_msg);
            }

            result[id[0]] = std::make_pair(pips_span, terminal);
        }
    }

    return result;
}

void xgrid::create_money_manager(const boost::json::object &transactions)
{
    std::string model = json_get_string_attribute(transactions, "model");

    if (model == "FLAT")
    {
        mm_flat_initializer mmfi;
        mmfi.number_of_lots_ = json_get_double_attribute(transactions, "number_of_lots");
        mmfi.remnant_svr_ = session_variable("xgrid.remnant");

        if (mmfi.number_of_lots_ <= 0.0)
        {
            std::string msg = "Attribute ‘transactions.number_of_lots’ must be greater than 0, got "
                              + std::to_string(mmfi.number_of_lots_);

            throw std::runtime_error(msg.c_str());
        }

        mmgmnt_.reset(new money_management(mmfi));
    }
    else if (model == "PROGRESSIVE")
    {
        mm_progressive_initializer mmpi;
        mmpi.slope_ = json_get_double_attribute(transactions, "slope");
        mmpi.remnant_svr_ = session_variable("xgrid.remnant");

        if (mmpi.slope_ <= 0.0)
        {
            std::string msg = "Attribute ‘transactions.slope’ must be greater than 0, got "
                              + std::to_string(mmpi.slope_);

            throw std::runtime_error(msg.c_str());
        }

        mmgmnt_.reset(new money_management(mmpi));
    }
    else
    {
        std::string msg = "Unrecognized value ‘" + model + "’ of attribute ‘transactions.model’";

        throw std::runtime_error(msg.c_str());
    }
}

void xgrid::parse_instrument_details(const boost::json::object &instrument_details)
{
    //
    // The following rules apply:
    //  – The values ​​of each field are floating-point numbers expressed
    //    in the currency of the broker account.
    //  – The value of 'commission_per_lot' is always positive even
    //    though it always incurs a cost.
    //  – The value of 'commission_per_lot' expresses the so-called
    //    double commission, i.e. it immediately covers the cost
    //    of opening and closing the position.
    //  – The value of 'long_dayswap_per_lot' is positive
    //    if it is an income and negative if it is an expense.
    //
    // Example:
    //
    // {
    //     "pip_value_per_lot":10.0,
    //     "margin_required_per_lot":211.0,
    //     "commission_per_lot":6.0,
    //     "long_dayswap_per_lot":-6.25
    // }
    //

    pip_value_per_lot_       = json_get_double_attribute(instrument_details, "pip_value_per_lot");
    margin_required_per_lot_ = json_get_double_attribute(instrument_details, "margin_required_per_lot");
    commission_per_lot_      = json_get_double_attribute(instrument_details, "commission_per_lot");
    long_dayswap_per_lot_    = json_get_double_attribute(instrument_details, "long_dayswap_per_lot");
}

void xgrid::create_grid(const boost::json::object &grid_def)
{
    std::string definition_type = json_get_string_attribute(grid_def, "definition_type");

    if (definition_type == "SIMPLE")
    {
        create_grid_simple_defined(grid_def);
    }
    else if (definition_type == "FULL")
    {
        create_grid_full_defined(grid_def);
    }
    else
    {
        std::string err_msg = "Unrecognized grid definition type ‘"
                              + definition_type + "’.";

        throw std::runtime_error(err_msg);
    }
}

void xgrid::create_grid_simple_defined(const boost::json::object &grid_def)
{
    //
    // Example:
    //
    // {
    //     "start_price" : 0.98765,
    //     "end_price" : 1.23456,
    //     "ncells" : 40,             /* Optionally, alternative to cell_pips_span */
    //     "cell_pips_span" : 20      /* Optionally, alternative to ncells */
    // }
    //

    //
    // Get ‘start_price’ attribute.
    //

    double start_price = json_get_double_attribute(grid_def, "start_price");

    if (start_price < 0.0)
    {
        std::string msg = "Attribute ‘start_price’ must not be negative value, got "
                          + std::to_string(json_get_double_attribute(grid_def, "start_price"));

        throw std::runtime_error(msg.c_str());
    }

    int start_price_pips = floating2pips(start_price);

    //
    // Get ‘end_price’ attribute.
    //

    double end_price = json_get_double_attribute(grid_def, "end_price");

    if (end_price < 0.0)
    {
        std::string msg = "Attribute ‘end_price’ must not be negative value, got "
                          + std::to_string(json_get_double_attribute(grid_def, "end_price"));

        throw std::runtime_error(msg.c_str());
    }

    int end_price_pips = floating2pips(end_price);

    int game_area = end_price_pips - start_price_pips;

    if (game_area < 1)
    {
        std::string msg = "Attribute ‘end_price’ must be greater than "
                          "‘start_price’ by at least one pips. Got start_price="
                          + std::to_string(start_price) + ", end_price="
                          + std::to_string(end_price);

        throw std::runtime_error(msg.c_str());
    }

    int cells_pips_span = 0;
    int ncells = 0;
    int r = 0;

    if (json_exist_attribute(grid_def, "ncells"))
    {
        ncells = json_get_int_attribute(grid_def, "ncells");

        if (ncells < 1)
        {
            std::string msg = "Attribute ‘ncells’ must be greater than 1, got "
                              + std::to_string(ncells);

            throw std::runtime_error(msg.c_str());
        }

        cells_pips_span = game_area / ncells;
        r = game_area % ncells;
    }
    else if (json_exist_attribute(grid_def, "cells_pips_span"))
    {
        cells_pips_span = json_get_int_attribute(grid_def, "cells_pips_span");

        if (cells_pips_span < 1)
        {
            std::string msg = "Attribute ‘cells_pips_span’ must be greater than 1, got "
                              + std::to_string(cells_pips_span);

            throw std::runtime_error(msg.c_str());
        }

        ncells = game_area / cells_pips_span;
        r = game_area % cells_pips_span;
    }
    else
    {
        throw std::runtime_error("Neither attribute ‘ncells’ nor attribute ‘cells_pips_span’ is defined");
    }

    int gnumber = 0;

    end_price_pips = start_price_pips + cells_pips_span + r;
    gcells_.emplace_back(opened_positions_counter_, positions_, start_price_pips, end_price_pips, gnumber++, false);

    for (int i = 1; i < ncells; i++)
    {
        start_price_pips = end_price_pips;
        end_price_pips += cells_pips_span;

        gcells_.emplace_back(opened_positions_counter_, positions_, start_price_pips, end_price_pips, gnumber++, false);
    }

    //
    // Terminal 400-pips-span extra cell.
    //

    gcells_.emplace_back(opened_positions_counter_, positions_, end_price_pips, end_price_pips + 400, gnumber++, true);
}

void xgrid::create_grid_full_defined(const boost::json::object &grid_def)
{
    //
    // Example:
    //
    //  {
    //      "cell_types": [
    //          {"class": "A", "pips_span": 50},
    //          {"class": "B", "pips_span": 100},
    //          {"class": "#", "pips_span": 180, "terminal": true}
    //      ],
    //      "start_price":15.00,
    //      "architecture":"AAAAAB##A#B#BBBBBB"
    //  }
    //

    //
    // Get ‘cell_types’ map.
    //

    auto cell_types = get_cell_types(grid_def);

    //
    // Get ‘start_price’ attribute.
    //

    double start_price = json_get_double_attribute(grid_def, "start_price");

    if (start_price < 0.0)
    {
        std::string msg = "Attribute ‘start_price’ must not be negative value, got "
                          + std::to_string(json_get_double_attribute(grid_def, "start_price"));

        throw std::runtime_error(msg.c_str());
    }

    int start_price_pips = floating2pips(start_price);

    //
    // Get ‘architecture’ attribute.
    //

    std::string architecture = json_get_string_attribute(grid_def, "architecture");

    //
    // Create grid.
    //

    int end_price_pips = 0;
    bool terminal = false;
    int gnumber = 0;

    for (int i = 0; i < architecture.length(); i++)
    {
        if (std::isspace(static_cast<unsigned char>(architecture[i])))
        {
            continue;
        }

        if (cell_types.count(architecture[i]) == 0)
        {
            std::string msg = "Undefined cell class ‘"
                              + std::string(1, architecture[i])
                              + std::string("’");

            throw std::runtime_error(msg.c_str());
        }

        end_price_pips = start_price_pips + cell_types[architecture[i]].first;
        terminal = cell_types[architecture[i]].second;

        gcells_.emplace_back(opened_positions_counter_, positions_, start_price_pips, end_price_pips, gnumber++, terminal);

        start_price_pips = end_price_pips;
    }

    if (gcells_.empty())
    {
        throw std::runtime_error("Grid is empty");
    }
}

void xgrid::load_positions(void)
{
    using namespace boost::json;

    if (! is_persistent())
    {
        return;
    }

    std::string json_data;

    try
    {
        json_data = file_get_contents("positions.json");
    }
    catch (const std::runtime_error &e)
    {
        hft_log(WARNING) << "load grid: Unalbe to load file ‘positions.json’";

        return;
    }

    value jv;

    try
    {
        jv = parse(json_data);
    }
    catch (const system_error &e)
    {
        throw std::runtime_error("Failed to parse file ‘positions.json’");
    }

    if (jv.kind() != kind::object)
    {
        throw std::runtime_error("Invalid file ‘positions.json’");
    }

    object const &obj = jv.get_object();

    std::string position_id;
    double position_volume;
    unsigned long position_timestamp;
    int position_price_pips;

    std::string candidate_obj_id;
    int x = 0;
    while (true)
    {
        candidate_obj_id = "g" + std::to_string(++x);

        if (! json_exist_attribute(obj, candidate_obj_id))
        {
            break;
        }

        const object &position_info_obj = json_get_object_attribute(obj, candidate_obj_id);

        position_id = json_get_string_attribute(position_info_obj, "id");

        if (position_id.length() > 0)
        {
            position_volume     = json_get_double_attribute(position_info_obj, "volume");
            position_timestamp  = boost::lexical_cast<unsigned long>(json_get_string_attribute(position_info_obj, "time"));
            position_price_pips = json_get_int_attribute(position_info_obj, "price_pips");

            hft_log(INFO) << "load positions: Attaching position ‘"
                          << position_id << "’ to set: (price pips: "
                          << position_price_pips << ", lots: "
                          << position_volume << ", opened: "
                          << hft::utils::timestamp2string(position_timestamp)
                          << ").";

            positions_.emplace_back(position_id, position_volume, position_timestamp, position_price_pips);
        }
    }

    hft_log(INFO) << "load positions: Total known positions: ‘"
                  << positions_.size() << "’";

    if (json_exist_attribute(obj, "user_alarmed"))
    {
        user_alarmed_ = json_get_bool_attribute(obj, "user_alarmed");
    }
}

void xgrid::save_positions(void)
{
    if (! is_persistent())
    {
        return;
    }

    std::ostringstream json_data;

    json_data << "{\n";

    json_data << "\t\"user_alarmed\":"
              << (user_alarmed_ ? "true" : "false")
              << ",\n";

    std::string candidate_obj_id;
    int x = 0;
    for (const position_record &p : positions_)
    {
        if (x > 0)
        {
            json_data << ",\n";
        }

        candidate_obj_id =  "g" + std::to_string(++x);

        //
        // {
        //     "g189":{"id":"hft_10123456a","volume":0.01,"time":1111111,"price_pips":87654},
        //     "g190":{"id":"hft_10234567b","volume":0.06,"time":2222222,"price_pips":76543}
        // }
        //

        json_data << "\t\"" << candidate_obj_id << "\":{\"id\":\""
                  << p.position_id_ << "\",\"volume\":"
                  << p.position_volume_ << ",\"time\":\""
                  << p.position_time_ << "\",\"price_pips\":"
                  << p.position_price_pips_ << "}";
    }

    json_data << "\n}\n";

    file_put_contents("positions.json", json_data.str());
}

void xgrid::verify_position_confirmation_status(void)
{
    if (positions_confirmed_)
    {
        return;
    }

    bool to_be_save = false;

    position_container::iterator it = positions_.begin();

    while (it != positions_.end())
    {
        if (! it -> position_confirmed_)
        {
            if (it -> position_id_ == "virtual")
            {
                it -> position_confirmed_ = true;
            }
            else
            {
                hft_log(WARNING) << "Position ‘" << (it -> position_id_)
                                 << "’ does not exist on market anymore, removing from set.";

                it = positions_.erase(it);

                to_be_save = true;

                continue;
            }
        }

        ++it;
    }

    //
    // Assign positions to the grid cells.
    //

    for (int i = 0; i < gcells_.size(); i++)
    {
        for (it = positions_.begin(); it != positions_.end(); it++)
        {
            if (! gcells_[i].is_terminal() && gcells_[i].inside_trading_zone(it -> position_price_pips_) && (it -> gcell_number_ < 0))
            {
                hft_log(INFO) << "Position ‘" << (it -> position_id_)
                              << "’ assigned to cell #" << i << ".";

                gcells_[i].assign_position(it);
                //it -> gcell_number_ = i;
            }
        }
    }

    for (auto &p : positions_)
    {
        if (p.gcell_number_ < 0)
        {
            hft_log(WARNING) << "Position ‘" << (p.position_id_) << "’"
                             << " Unmanaged – not assigned to any cell.";
        }
    }

    if (to_be_save)
    {
        save_positions();
    }

    positions_confirmed_ = true;
}

void xgrid::await_position_status(void)
{
    static const int max_aps_counter_value = 60;

    if (current_state_ == state::WAIT_FOR_STATUS)
    {
        awaiting_position_status_counter_++;

        if (awaiting_position_status_counter_ > max_aps_counter_value)
        {
            auto it = positions_.begin();

            while (it != positions_.end())
            {
                if (! it -> position_confirmed_)
                {
                    auto it2 = it;
                    ++it2;

                    hft_log(WARNING) << "Position ‘" << (it -> position_id_)
                                     << "’ managed by gcell #" << (it -> gcell_number_)
                                     << " has not been confirmed to be existent by broker within defined time, removing.";

                    gcells_[it -> gcell_number_].detatch_position(it);

                    it = it2;
                }
                else
                {
                    ++it;
                }
            }

            awaiting_position_status_counter_ = 0;
            current_state_ = state::OPERATIONAL;
        }
    }
}

} /* namespace hft_ih_plugin */
