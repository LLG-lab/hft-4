#include <trend_tracker.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, get_logger_id().c_str())

namespace hft_ih_plugin {

trend_tracker::trend_tracker(const instrument_handler::init_info &general_config)
    : instrument_handler(general_config),
      max_spread_ {10000000},
      num_of_contracts_ {0},
      position_confirmed_ {false},
      awaiting_position_status_counter_ {0}
{
    el::Loggers::getLogger(get_logger_id().c_str(), true);

    hft_log(INFO) << "Starting instrument handler ‘Trend Tracker™’ for instrument ‘"
                  << get_ticker() << "’, i.e. ‘"
                  << get_instrument_description()
                  << "’";

    //
    // Setup default informations about position.
    //

    hs_.set_int_var("position.state", (int) state::IDLE);
    hs_.set_int_var("position.open_price_pips", 0);
    hs_.set_int_var("position.pips_limit", 0);
    hs_.set_bool_var("position.close_attempted", false);
    hs_.set_string_var("position.id", "");
    hs_.persistent();
}

//
// Public interface.
//

void trend_tracker::init_handler(const boost::json::object &specific_config)
{
    //
    // Going to obtain following attributes (example):
    //

    //
    // "handler_options": {
    //     "max_spread":5, /* optional */
    //     "contracts":10,
    //     "est_probab_threshold":0.65,
    //     "m5":{"depths":[1,2,3,4,5],"pips_limits":[1,2,3,4,5]},
    //     "h1":{"depths":[1,2,3,4,5],"pips_limits":[1,2,3,4,5]}
    // }
    //

    std::vector<int> depths;
    std::vector<int> pips_limits;
    double est_probab_threshold;

    try
    {
        num_of_contracts_ = json_get_int_attribute(specific_config, "contracts");

        if (num_of_contracts_ <= 0)
        {
            std::string msg = "Attribute ‘contracts’ must be greater than 0, got "
                              + std::to_string(num_of_contracts_);

            throw std::runtime_error(msg.c_str());
        }

        est_probab_threshold = json_get_double_attribute(specific_config, "est_probab_threshold");

        if (est_probab_threshold <= 0.0 || est_probab_threshold >= 1.0)
        {
            std::string msg = "Attribute ‘est_probab_threshold’ must be within (0..1), got "
                              + std::to_string(est_probab_threshold);

            throw std::runtime_error(msg.c_str());
        }

        if (json_exist_attribute(specific_config, "max_spread"))
        {
            max_spread_ = json_get_int_attribute(specific_config, "max_spread");

            if (max_spread_ < 0)
            {
                std::string msg = "Attribute ‘max_spread’ must be greater than 0, got "
                                  + std::to_string(max_spread_);

                throw std::runtime_error(msg.c_str());
            }
        }

        //
        // Create strategic engine, and configure it.
        //

        strategy_.reset(new strategic_engine(get_logger_id(), get_work_dir(), est_probab_threshold));

        //
        // Obtain informations for strategic engine's
        // interval processors.
        //

        if (json_exist_attribute(specific_config, "m1"))
        {
            const boost::json::object &m1 = json_get_object_attribute(specific_config, "m1");
            depths = json_get_int_array_attribute(m1, "depths");
            pips_limits = json_get_int_array_attribute(m1, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_M1, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "m2"))
        {
            const boost::json::object &m2 = json_get_object_attribute(specific_config, "m2");
            depths = json_get_int_array_attribute(m2, "depths");
            pips_limits = json_get_int_array_attribute(m2, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_M2, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "m5"))
        {
            const boost::json::object &m5 = json_get_object_attribute(specific_config, "m5");
            depths = json_get_int_array_attribute(m5, "depths");
            pips_limits = json_get_int_array_attribute(m5, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_M5, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "m10"))
        {
            const boost::json::object &m10 = json_get_object_attribute(specific_config, "m10");
            depths = json_get_int_array_attribute(m10, "depths");
            pips_limits = json_get_int_array_attribute(m10, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_M10, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "m15"))
        {
            const boost::json::object &m15 = json_get_object_attribute(specific_config, "m15");
            depths = json_get_int_array_attribute(m15, "depths");
            pips_limits = json_get_int_array_attribute(m15, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_M15, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "m20"))
        {
            const boost::json::object &m20 = json_get_object_attribute(specific_config, "m20");
            depths = json_get_int_array_attribute(m20, "depths");
            pips_limits = json_get_int_array_attribute(m20, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_M20, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "m30"))
        {
            const boost::json::object &m30 = json_get_object_attribute(specific_config, "m30");
            depths = json_get_int_array_attribute(m30, "depths");
            pips_limits = json_get_int_array_attribute(m30, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_M30, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "h1"))
        {
            const boost::json::object &h1 = json_get_object_attribute(specific_config, "h1");
            depths = json_get_int_array_attribute(h1, "depths");
            pips_limits = json_get_int_array_attribute(h1, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_H1, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "h2"))
        {
            const boost::json::object &h2 = json_get_object_attribute(specific_config, "h2");
            depths = json_get_int_array_attribute(h2, "depths");
            pips_limits = json_get_int_array_attribute(h2, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_H2, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "h3"))
        {
            const boost::json::object &h3 = json_get_object_attribute(specific_config, "h3");
            depths = json_get_int_array_attribute(h3, "depths");
            pips_limits = json_get_int_array_attribute(h3, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_H3, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "h4"))
        {
            const boost::json::object &h4 = json_get_object_attribute(specific_config, "h4");
            depths = json_get_int_array_attribute(h4, "depths");
            pips_limits = json_get_int_array_attribute(h4, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_H4, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "h6"))
        {
            const boost::json::object &h6 = json_get_object_attribute(specific_config, "h6");
            depths = json_get_int_array_attribute(h6, "depths");
            pips_limits = json_get_int_array_attribute(h6, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_H6, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "h8"))
        {
            const boost::json::object &h8 = json_get_object_attribute(specific_config, "h8");
            depths = json_get_int_array_attribute(h8, "depths");
            pips_limits = json_get_int_array_attribute(h8, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_H8, depths, pips_limits);
        }

        if (json_exist_attribute(specific_config, "h12"))
        {
            const boost::json::object &h12 = json_get_object_attribute(specific_config, "h12");
            depths = json_get_int_array_attribute(h12, "depths");
            pips_limits = json_get_int_array_attribute(h12, "pips_limits");

            strategy_ -> configure_processors(interval_t::I_H12, depths, pips_limits);
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

void trend_tracker::on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market)
{
    if (position_confirmed_)
    {
        return;
    }

    std::string position_id = hs_.get_string_var("position.id");

    if (position_id == "")
    {
        auto as = hs_.create_autosaver();

        hs_.set_int_var("position.state", (int) state::IDLE);
        hs_.set_int_var("position.open_price_pips", 0);
        hs_.set_int_var("position.pips_limit", 0);
        hs_.set_bool_var("position.close_attempted", false);

        position_confirmed_ = true;

        return;
    }

    if (position_id != msg.id)
    {
        hft_log(INFO) << "Ignoring opened position ‘" << msg.id << "’.";

        return;
    }

    int open_price_pips = floating2pips(msg.price);
    int direction = hs_.get_int_var("position.state");

    if (direction == (int) state::LONG ||
            direction == (int) state::TRY_CLOSE_LONG ||
                direction == (int) state::TRY_OPEN_LONG)
    {
        if (! msg.is_long)
        {
            hft_log(ERROR) << "sync: Bad direction for position ‘" << position_id
                           << "’. Expected ‘LONG’, got ‘SHORT’.";

            throw std::runtime_error("Position synchronization error");
        }

        auto as = hs_.create_autosaver();

        hs_.set_int_var("position.state", (int) (int) state::LONG);
        hs_.set_int_var("position.open_price_pips", open_price_pips);
        hs_.set_bool_var("position.close_attempted", false);

        hft_log(INFO) << "Synchronized LONG position ‘"
                      << position_id << "’, open price ‘"
                      << msg.price << "’";
    }
    else if (direction == (int) state::SHORT ||
                 direction == (int) state::TRY_CLOSE_SHORT ||
                     direction == (int) state::TRY_OPEN_SHORT)
    {
        if (msg.is_long)
        {
            hft_log(ERROR) << "sync: Bad direction for position ‘" << position_id
                           << "’. Expected ‘LONG’, got ‘SHORT’.";

            throw std::runtime_error("Position synchronization error");
        }

        auto as = hs_.create_autosaver();

        hs_.set_int_var("position.state", (int) (int) state::SHORT);
        hs_.set_int_var("position.open_price_pips", open_price_pips);
        hs_.set_bool_var("position.close_attempted", false);

        hft_log(INFO) << "Synchronized SHORT position ‘"
                      << position_id << "’, open price ‘"
                      << msg.price << "’";

    }

    position_confirmed_ = true;
}

void trend_tracker::on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market)
{
    verify_position_confirmation_status();

    auto as = hs_.create_autosaver();

    int ask_pips = floating2pips(msg.ask);
    int bid_pips = floating2pips(msg.bid);

    strategy_ -> tick(ask_pips, bid_pips, msg.request_time);

    state position_state = (state) hs_.get_int_var("position.state");
    switch (position_state)
    {
        case state::IDLE:
        {
            investment_advice ia = strategy_ -> get_advice();

            if (ia.decision == decision_t::E_LONG)
            {
                hs_.set_int_var("position.state", (int) state::TRY_OPEN_LONG);
                hs_.set_int_var("position.open_price_pips", ask_pips);
                hs_.set_string_var("position.id", uid());
                hs_.set_bool_var("position.close_attempted", false);
                hs_.set_int_var("position.pips_limit", ia.pips_limit);

                market.open_long(hs_.get_string_var("position.id"), num_of_contracts_);

                hft_log(INFO) << "(idle) Trying to take position LONG with num of contracts ‘"
                              << num_of_contracts_ << "’, ID is ‘" << hs_.get_string_var("position.id")
                              << "’, equity now is ‘" << msg.equity << "’, expected open price is ‘"
                              << msg.ask << "’, limit is " << ia.pips_limit << " pips.";
            }
            else if (ia.decision == decision_t::E_SHORT)
            {
                hs_.set_int_var("position.state", (int) state::TRY_OPEN_SHORT);
                hs_.set_int_var("position.open_price_pips", bid_pips);
                hs_.set_string_var("position.id", uid());
                hs_.set_bool_var("position.close_attempted", false);
                hs_.set_int_var("position.pips_limit", ia.pips_limit);

                market.open_short(hs_.get_string_var("position.id"), num_of_contracts_);

                hft_log(INFO) << "(idle) Trying to take position SHORT with num of contracts ‘"
                              << num_of_contracts_ << "’, ID is ‘" << hs_.get_string_var("position.id")
                              << "’, equity now is ‘" << msg.equity << "’, expected open price is ‘"
                              << msg.bid << "’, limit is " << ia.pips_limit << " pips.";
            }

            break;
        }
        case state::LONG:
        {
            continue_long_position(ask_pips, bid_pips, market);
            break;
        }
        case state::SHORT:
        {
            continue_short_position(ask_pips, bid_pips, market);
            break;
        }
        case state::TRY_OPEN_LONG:
        case state::TRY_OPEN_SHORT:
        case state::TRY_CLOSE_LONG:
        case state::TRY_CLOSE_SHORT:
            await_position_status();
    }
}

void trend_tracker::on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market)
{
    auto as = hs_.create_autosaver();

    state position_state = (state) hs_.get_int_var("position.state");

    if (msg.status)
    {
        switch (position_state)
        {
            case state::TRY_OPEN_LONG:
                if (msg.id == hs_.get_string_var("position.id"))
                {
                    hs_.set_int_var("position.state", (int) state::LONG);
                    hs_.set_int_var("position.open_price_pips", floating2pips(msg.price));

                    hft_log(INFO) << "(LONG) Successfuly opened position, open price was ‘"
                                  << msg.price << "’.";
                }
                else
                {
                    hft_log(ERROR) << "position open: Position ID missmatch: expected ‘"
                                   << hs_.get_string_var("position.id") << "’, got ‘"
                                   << msg.id << "’ – ignoring.";
                }

                break;
            case state::TRY_OPEN_SHORT:
                if (msg.id == hs_.get_string_var("position.id"))
                {
                    hs_.set_int_var("position.state", (int) state::SHORT);
                    hs_.set_int_var("position.open_price_pips", floating2pips(msg.price));

                    hft_log(INFO) << "(SHORT) Successfuly opened position, open price was ‘"
                                  << msg.price << "’.";
                }
                else
                {
                    hft_log(ERROR) << "position open: Position ID missmatch: expected ‘"
                                   << hs_.get_string_var("position.id") << "’, got ‘"
                                   << msg.id << "’ – ignoring.";
                }

                break;
            default:
                hft_log(ERROR) << "Unexpected position open notify";
        }
    }
    else // msg.status
    {
        switch (position_state)
        {
            case state::TRY_OPEN_LONG:
            case state::TRY_OPEN_SHORT:
                if (msg.id == hs_.get_string_var("position.id"))
                {
                    hs_.set_int_var("position.state", (int) state::IDLE);
                    hs_.set_int_var("position.open_price_pips", 0);
                    hs_.set_int_var("position.pips_limit", 0);
                    hs_.set_string_var("position.id", "");

                    hft_log(INFO) << "(idle) Failed to open position.";
                }
                else
                {
                    hft_log(ERROR) << "position open: Position ID missmatch: expected ‘"
                                   << hs_.get_string_var("position.id") << "’, got ‘"
                                   << msg.id << "’ – ignoring.";
                }

                break;
            default:
                hft_log(ERROR) << "Unexpected position open notify";
        }
    }
}

void trend_tracker::on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market)
{
    auto as = hs_.create_autosaver();

    state position_state = (state) hs_.get_int_var("position.state");

    if (msg.status)
    {
        switch (position_state)
        {
            case state::TRY_CLOSE_SHORT:
            case state::TRY_CLOSE_LONG:
                if (msg.id == hs_.get_string_var("position.id"))
                {
                    hs_.set_int_var("position.state", (int) state::IDLE);
                    hs_.set_int_var("position.open_price_pips", 0);
                    hs_.set_int_var("position.pips_limit", 0);
                    hs_.set_bool_var("position.close_attempted", false);
                    hs_.set_string_var("position.id", "");

                    hft_log(INFO) << "(idle) Position ‘" << msg.id
                                  << "’ closed successfuly.";
                }
                else
                {
                    hft_log(ERROR) << "position close: Position ID missmatch: expected ‘"
                                   << hs_.get_string_var("position.id") << "’, got ‘"
                                   << msg.id << "’ – ignoring.";
                }

                break;
            default:
                hft_log(ERROR) << "Unexpected position close notify";
        }
    }
    else // msg.status
    {
        switch (position_state)
        {
            case state::TRY_CLOSE_SHORT:
                if (msg.id == hs_.get_string_var("position.id"))
                {
                    hs_.set_int_var("position.state", (int) state::SHORT);
                    hs_.set_bool_var("position.close_attempted", true);

                    hft_log(INFO) << "(SHORT) Failed to close position ‘"
                                  << hs_.get_string_var("position.id") << "’.";
                }
                else
                {
                    hft_log(ERROR) << "position close: Position ID missmatch: expected ‘"
                                   << hs_.get_string_var("position.id") << "’, got ‘"
                                   << msg.id << "’ – ignoring.";
                }

                break;
            case state::TRY_CLOSE_LONG:
                if (msg.id == hs_.get_string_var("position.id"))
                {
                    hs_.set_int_var("position.state", (int) state::LONG);
                    hs_.set_bool_var("position.close_attempted", true);

                    hft_log(INFO) << "(LONG) Failed to close position ‘"
                                  << hs_.get_string_var("position.id") << "’.";
                }
                else
                {
                    hft_log(ERROR) << "position close: Position ID missmatch: expected ‘"
                                   << hs_.get_string_var("position.id") << "’, got ‘"
                                   << msg.id << "’ – ignoring.";
                }

                break;
            default:
                hft_log(ERROR) << "Unexpected position close notify";
        }
    }
}

//
// Private methods.
//

void trend_tracker::continue_long_position(int ask_pips, int bid_pips, hft::protocol::response &market)
{
    int spread = ask_pips - bid_pips;

    if (spread > max_spread_)
    {
        hft_log(TRACE) << "(LONG) Spread to high: "
                       << spread << " pips.";

        return;
    }

    if (hs_.get_bool_var("position.close_attempted"))
    {
        hft_log(WARNING) << "(LONG) Reattempting close position ‘"
                         << hs_.get_string_var("position.id") << "’.";

        hs_.set_int_var("position.state", (int) state::TRY_CLOSE_LONG);

        market.close_position(hs_.get_string_var("position.id"));

        return;
    }

    int trade_pips_limit = hs_.get_int_var("position.pips_limit");
    int yield = bid_pips - hs_.get_int_var("position.open_price_pips");

    if (abs(yield) >= trade_pips_limit)
    {
        hft_log(INFO) << "(LONG) Setup accomplished with yield: "
                      << yield << " pips.";

        investment_advice ia = strategy_ -> get_advice();

        if (ia.decision == decision_t::E_LONG)
        {
            hs_.set_int_var("position.open_price_pips", ask_pips);
            hs_.set_int_var("position.pips_limit", ia.pips_limit);

            hft_log(INFO) << "(LONG) Position ‘" << hs_.get_string_var("position.id")
                          << "’ remains for new setup. Open price (in pips) is ‘"
                          << ask_pips << "’, limit is " << ia.pips_limit << " pips.";
        }
        else
        {
            hft_log(INFO) << "(LONG) Going to close position ‘"
                          << hs_.get_string_var("position.id") << "’.";

            hs_.set_int_var("position.state", (int) state::TRY_CLOSE_LONG);

            market.close_position(hs_.get_string_var("position.id"));
        }
    }
}

void trend_tracker::continue_short_position(int ask_pips, int bid_pips, hft::protocol::response &market)
{
    int spread = ask_pips - bid_pips;

    if (spread > max_spread_)
    {
        hft_log(TRACE) << "(SHORT) Spread to high: "
                       << spread << " pips.";

        return;
    }

    if (hs_.get_bool_var("position.close_attempted"))
    {
        hft_log(WARNING) << "(SHORT) Reattempting close position ‘"
                         << hs_.get_string_var("position.id") << "’.";

         hs_.set_int_var("position.state", (int) state::TRY_CLOSE_SHORT);

         market.close_position(hs_.get_string_var("position.id"));

         return;
    }

    int trade_pips_limit = hs_.get_int_var("position.pips_limit");
    int yield = hs_.get_int_var("position.open_price_pips") - ask_pips;

    if (abs(yield) >= trade_pips_limit)
    {
        hft_log(INFO) << "(SHORT) Setup accomplished with yield: "
                      << yield << " pips.";

        investment_advice ia = strategy_ -> get_advice();

        if (ia.decision == decision_t::E_SHORT)
        {
            hs_.set_int_var("position.open_price_pips", bid_pips);
            hs_.set_int_var("position.pips_limit", ia.pips_limit);

            hft_log(INFO) << "(SHORT) Position ‘" << hs_.get_string_var("position.id")
                          << "’ remains for new setup. Open price (in pips) is ‘"
                          << bid_pips << "’, limit is " << ia.pips_limit << " pips.";
        }
        else
        {
            hft_log(INFO) << "(SHORT) Going to close position ‘"
                          << hs_.get_string_var("position.id") << "’.";

            hs_.set_int_var("position.state", (int) state::TRY_CLOSE_SHORT);

            market.close_position(hs_.get_string_var("position.id"));
        }
    }
}

std::string trend_tracker::state2state_str(state position_state)
{
    switch ((state) position_state)
    {
        case state::IDLE:
            return "IDLE";
        case state::TRY_OPEN_SHORT:
            return "TRY_OPEN_SHORT";
        case state::SHORT:
            return "SHORT";
        case state::TRY_OPEN_LONG:
            return "TRY_OPEN_LONG";
        case state::LONG:
            return "LONG";
        case state::TRY_CLOSE_SHORT:
            return "TRY_CLOSE_SHORT";
        case state::TRY_CLOSE_LONG:
            return "TRY_CLOSE_LONG";
    }

    return "???";
}

void trend_tracker::verify_position_confirmation_status(void)
{
    if (position_confirmed_)
    {
        return;
    }

    auto as = hs_.create_autosaver();

    std::string position_state = state2state_str(hs_.get_int_var("position.state"));
    
    hft_log(WARNING) << "Position ‘" << hs_.get_string_var("position.id")
                     << "’ is missing. Last state was ‘" << position_state
                     << "’. Assume it was closed manually.";

    hs_.set_int_var("position.state", (int) state::IDLE);
    hs_.set_int_var("position.open_price_pips", 0);
    hs_.set_int_var("position.pips_limit", 0);
    hs_.set_bool_var("position.close_attempted", false);
    hs_.set_string_var("position.id", "");

    position_confirmed_ = true;
}

void trend_tracker::await_position_status(void)
{
    static const int max_aps_counter_value = 60;

    state position_state = (state) hs_.get_int_var("position.state");

    awaiting_position_status_counter_++;

    switch (position_state)
    {
        case state::TRY_OPEN_LONG:
        {
            if (awaiting_position_status_counter_ > max_aps_counter_value)
            {
                // auto as = hs_.create_autosaver(); XXX Nie potrzeba, bo on_tick już ma autosavera

                hft_log(INFO) << "(try open long) Status notification not received for position ‘"
                              << hs_.get_string_var("position.id")
                              << "’ within defined time – reverting TRY_OPEN_LONG → IDLE.";

                hs_.set_int_var("position.state", (int) state::IDLE);
                hs_.set_int_var("position.open_price_pips", 0);
                hs_.set_bool_var("position.close_attempted", false);
                hs_.set_string_var("position.id", "");

                awaiting_position_status_counter_ = 0;
            }
            else
            {
                hft_log(INFO) << "(try open long) Awaiting for position status ‘"
                              << hs_.get_string_var("position.id") << "’.";
            }

            break;
        }
        case state::TRY_OPEN_SHORT:
        {
            if (awaiting_position_status_counter_ > max_aps_counter_value)
            {
                // auto as = hs_.create_autosaver(); XXX Nie potrzeba, bo on_tick już ma autosavera

                hft_log(INFO) << "(try open short) Status notification not received for position ‘"
                              << hs_.get_string_var("position.id")
                              << "’ within defined time – reverting TRY_OPEN_SHORT → IDLE.";

                hs_.set_int_var("position.state", (int) state::IDLE);
                hs_.set_int_var("position.open_price_pips", 0);
                hs_.set_bool_var("position.close_attempted", false);
                hs_.set_string_var("position.id", "");

                awaiting_position_status_counter_ = 0;
            }
            else
            {
                hft_log(INFO) << "(try open short) Awaiting for position status ‘"
                              << hs_.get_string_var("position.id") << "’.";
            }

            break;
        }
        case state::TRY_CLOSE_LONG:
        {
            if (awaiting_position_status_counter_ > max_aps_counter_value)
            {
                // auto as = hs_.create_autosaver(); XXX Nie potrzeba, bo on_tick już ma autosavera

                hft_log(INFO) << "(try close long) Status notification not received for position ‘"
                              << hs_.get_string_var("position.id")
                              << "’ within defined time – reverting TRY_CLOSE_LONG → LONG.";

                hs_.set_int_var("position.state", (int) state::LONG);
                hs_.set_bool_var("position.close_attempted", false);

                awaiting_position_status_counter_ = 0;
            }
            else
            {
                hft_log(INFO) << "(try close long) Awaiting for position status ‘"
                              << hs_.get_string_var("position.id") << "’.";
            }

            break;
        }
        case state::TRY_CLOSE_SHORT:
        {
            if (awaiting_position_status_counter_ > max_aps_counter_value)
            {
                // auto as = hs_.create_autosaver(); XXX Nie potrzeba, bo on_tick już ma autosavera

                hft_log(INFO) << "(try close long) Status notification not received for position ‘"
                              << hs_.get_string_var("position.id")
                              << "’ within defined time – reverting TRY_CLOSE_SHORT → SHORT.";

                hs_.set_int_var("position.state", (int) state::SHORT);
                hs_.set_bool_var("position.close_attempted", false);

                awaiting_position_status_counter_ = 0;
            }
            else
            {
                hft_log(INFO) << "(try close short) Awaiting for position status ‘"
                              << hs_.get_string_var("position.id") << "’.";
            }

            break;
        }
    }
}


} /* namespace hft_ih_plugin */
