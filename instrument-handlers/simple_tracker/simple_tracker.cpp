#include <simple_tracker.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, get_logger_id().c_str())

namespace hft_ih_plugin {

simple_tracker::simple_tracker(const instrument_handler::init_info &general_config)
    : instrument_handler(general_config),
      play_decision_ {decision::OUT_OF_MARKET},
      num_of_contracts_ {0},
      max_pips_loss_ {0},
      position_confirmed_ {false},
      awaiting_position_status_counter_ {0}
{
    el::Loggers::getLogger(get_logger_id().c_str(), true);

    hft_log(INFO) << "Starting instrument handler ‘Simple Tracker™’ for instrument ‘"
                  << get_ticker() << "’, i.e. ‘"
                  << get_instrument_description()
                  << "’";

    //
    // Setup default informations about position.
    //

    hs_.set_int_var("position.state", (int) state::IDLE);
    hs_.set_int_var("position.reference_price_pips", 0);
    hs_.set_bool_var("position.close_attempted", false);
    hs_.set_string_var("position.id", "");
}

//
// Public interface.
//

void simple_tracker::init_handler(const boost::json::object &specific_config)
{
    //
    // Going to obtain following attributes (example):
    //

    //
    // "handler_options": {
    //     "contracts":10,
    //     "max_pips_loss":50000,
    //     "default_bet":"LONG", /* optional */
    //     "persistent":true     /* optional */
    // }
    //

    try
    {
        num_of_contracts_ = json_get_int_attribute(specific_config, "contracts");

        if (num_of_contracts_ <= 0)
        {
            std::string msg = "Attribute ‘contracts’ must be greater than 0, got "
                              + std::to_string(num_of_contracts_);

            throw std::runtime_error(msg.c_str());
        }

        max_pips_loss_ = json_get_int_attribute(specific_config, "max_pips_loss");

        if (max_pips_loss_ <= 0)
        {
            std::string msg = "Attribute ‘max_pips_loss’ must be greater than 0, got "
                              + std::to_string(max_pips_loss_);

            throw std::runtime_error(msg.c_str());
        }

        if (json_exist_attribute(specific_config, "default_bet"))
        {
            std::string default_bet_str = json_get_string_attribute(specific_config, "default_bet");

            if (default_bet_str == "LONG" || default_bet_str == "long")
            {
                play_decision_ = decision::PLAY_LONG;
            }
            else if (default_bet_str == "SHORT" || default_bet_str == "short")
            {
                play_decision_ = decision::PLAY_SHORT;
            }
            else
            {
                std::string msg = "Illegal value of ‘default_bet’ attribute: "
                                  + default_bet_str;

                throw std::runtime_error(msg.c_str());
            }
        }
        else
        {
            hft_log(WARNING) << "init: Undefined ‘default_bet’ attribute.";
        }

        if (json_exist_attribute(specific_config, "persistent"))
        {
            if (json_get_bool_attribute(specific_config, "persistent"))
            {
                hs_.persistent();

                hft_log(INFO) << "init: Handler is persistent";
            }
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

void simple_tracker::on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market)
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
        hs_.set_int_var("position.reference_price_pips", 0);
        hs_.set_bool_var("position.close_attempted", false);

        position_confirmed_ = true;

        return;
    }

    if (position_id != msg.id)
    {
        hft_log(INFO) << "Ignoring opened position ‘" << msg.id << "’.";

        return;
    }

    // XXX int open_price_pips = floating2pips(msg.price);
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
        //XXX: hs_.set_int_var("position.open_price_pips", open_price_pips);
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
        //XXX: hs_.set_int_var("position.open_price_pips", open_price_pips);
        hs_.set_bool_var("position.close_attempted", false);

        hft_log(INFO) << "Synchronized SHORT position ‘"
                      << position_id << "’, open price ‘"
                      << msg.price << "’";

    }

    position_confirmed_ = true;
}

void simple_tracker::on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market)
{
    verify_position_confirmation_status();

    if (! can_play(msg.request_time))
    {
        return;
    }

    auto as = hs_.create_autosaver();

    int ask_pips = floating2pips(msg.ask);
    int bid_pips = floating2pips(msg.bid);

    state position_state = (state) hs_.get_int_var("position.state");

    switch (position_state)
    {
        case state::IDLE:
        {
            if (play_decision_ == decision::PLAY_LONG)
            {
                hs_.set_int_var("position.state", (int) state::TRY_OPEN_LONG);
                hs_.set_int_var("position.reference_price_pips", ask_pips);
                hs_.set_string_var("position.id", uid());
                hs_.set_bool_var("position.close_attempted", false);

                market.open_long(hs_.get_string_var("position.id"), num_of_contracts_);

                hft_log(INFO) << "(idle) Trying to take position LONG with num of contracts ‘"
                              << num_of_contracts_ << "’, ID is ‘" << hs_.get_string_var("position.id")
                              << "’, equity now is ‘" << msg.equity << "’, expected open price is ‘"
                              << msg.ask << "’, request time is ‘"
                              << boost::posix_time::to_simple_string(msg.request_time) << "’.";

            }
            else if (play_decision_ == decision::PLAY_SHORT)
            {
                hs_.set_int_var("position.state", (int) state::TRY_OPEN_SHORT);
                hs_.set_int_var("position.reference_price_pips", bid_pips);
                hs_.set_string_var("position.id", uid());
                hs_.set_bool_var("position.close_attempted", false);

                market.open_short(hs_.get_string_var("position.id"), num_of_contracts_);

                hft_log(INFO) << "(idle) Trying to take position SHORT with num of contracts ‘"
                              << num_of_contracts_ << "’, ID is ‘" << hs_.get_string_var("position.id")
                              << "’, equity now is ‘" << msg.equity << "’, expected open price is ‘"
                              << msg.bid << "’, request time is ‘"
                              << boost::posix_time::to_simple_string(msg.request_time) << "’.";
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

void simple_tracker::on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market)
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
                    hs_.set_int_var("position.reference_price_pips", floating2pips(msg.price));

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
                    hs_.set_int_var("position.reference_price_pips", floating2pips(msg.price));

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
                    hs_.set_int_var("position.reference_price_pips", 0);
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

void simple_tracker::on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market)
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
                    hs_.set_int_var("position.reference_price_pips", 0);
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

void simple_tracker::continue_long_position(int ask_pips, int bid_pips, hft::protocol::response &market)
{
    if (hs_.get_bool_var("position.close_attempted"))
    {
        hft_log(WARNING) << "(LONG) Reattempting close position ‘"
                         << hs_.get_string_var("position.id") << "’.";

        hs_.set_int_var("position.state", (int) state::TRY_CLOSE_LONG);

        market.close_position(hs_.get_string_var("position.id"));

        play_decision_ = decision::PLAY_SHORT;

        return;
    }

    int reference_price_pips = hs_.get_int_var("position.reference_price_pips");

    if (reference_price_pips - bid_pips >= max_pips_loss_)
    {
        hft_log(INFO) << "(LONG) Going to close position ‘"
                      << hs_.get_string_var("position.id") << "’.";

        hs_.set_int_var("position.state", (int) state::TRY_CLOSE_LONG);

        market.close_position(hs_.get_string_var("position.id"));

        play_decision_ = decision::PLAY_SHORT;

        return;
    }

    if (ask_pips > reference_price_pips)
    {
        hs_.set_int_var("position.reference_price_pips", ask_pips);
    }
}

void simple_tracker::continue_short_position(int ask_pips, int bid_pips, hft::protocol::response &market)
{
    if (hs_.get_bool_var("position.close_attempted"))
    {
        hft_log(WARNING) << "(SHORT) Reattempting close position ‘"
                         << hs_.get_string_var("position.id") << "’.";

         hs_.set_int_var("position.state", (int) state::TRY_CLOSE_SHORT);

         market.close_position(hs_.get_string_var("position.id"));

         play_decision_ = decision::PLAY_LONG;

         return;
    }

    int reference_price_pips = hs_.get_int_var("position.reference_price_pips");

    if (ask_pips - reference_price_pips >= max_pips_loss_)
    {
        hft_log(WARNING) << "(SHORT) Going to close position ‘"
                         << hs_.get_string_var("position.id") << "’.";

         hs_.set_int_var("position.state", (int) state::TRY_CLOSE_SHORT);

         market.close_position(hs_.get_string_var("position.id"));

         play_decision_ = decision::PLAY_LONG;

         return;
    }

    if (bid_pips < reference_price_pips)
    {
        hs_.set_int_var("position.reference_price_pips", bid_pips);
    }
}

std::string simple_tracker::state2state_str(state position_state)
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

void simple_tracker::verify_position_confirmation_status(void)
{
    if (position_confirmed_)
    {
        return;
    }

    auto as = hs_.create_autosaver();

    if (hs_.get_string_var("position.id") != "")
    {
        std::string position_state = state2state_str(hs_.get_int_var("position.state"));
    
        hft_log(WARNING) << "Position ‘" << hs_.get_string_var("position.id")
                         << "’ is missing. Last state was ‘" << position_state
                         << "’. Assume it was closed manually.";
    }

    hs_.set_int_var("position.state", (int) state::IDLE);
    hs_.set_int_var("position.reference_price_pips", 0);
    hs_.set_bool_var("position.close_attempted", false);
    hs_.set_string_var("position.id", "");

    position_confirmed_ = true;
}

void simple_tracker::await_position_status(void)
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
                hft_log(INFO) << "(try open long) Status notification not received for position ‘"
                              << hs_.get_string_var("position.id")
                              << "’ within defined time – reverting TRY_OPEN_LONG → IDLE.";

                hs_.set_int_var("position.state", (int) state::IDLE);
                hs_.set_int_var("position.reference_price_pips", 0);
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
                hft_log(INFO) << "(try open short) Status notification not received for position ‘"
                              << hs_.get_string_var("position.id")
                              << "’ within defined time – reverting TRY_OPEN_SHORT → IDLE.";

                hs_.set_int_var("position.state", (int) state::IDLE);
                hs_.set_int_var("position.reference_price_pips", 0);
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
