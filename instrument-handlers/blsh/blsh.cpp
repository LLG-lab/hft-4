#include <blsh.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, get_logger_id().c_str())

namespace hft_ih_plugin {

blsh::blsh(const instrument_handler::init_info &general_config)
    : instrument_handler(general_config),
      contracts_ {0},
      max_spread_ {0},
      tau_ {0},
      position_confirmed_ {false},
      awaiting_position_status_counter_ {0}
{
    el::Loggers::getLogger(get_logger_id().c_str(), true);

    hft_log(INFO) << "Starting instrument handler ‘BLSH – Buy Low Sell High™’ for instrument ‘"
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

void blsh::init_handler(const boost::json::object &specific_config)
{
    //
    // Going to obtain following attributes (example):
    //

    //
    //  "handler_options": {
    //      "persistent": false,
    //      "contracts": 1,
    //      "max_spread": 12,
    //      "tau": 86400
    //  }
    //

    try
    {
        contracts_ = json_get_int_attribute(specific_config, "contracts");

        if (contracts_ <= 0)
        {
            std::string msg = "Attribute ‘contracts’ must be greater than 0, got "
                              + std::to_string(contracts_);

            throw std::runtime_error(msg.c_str());
        }

        max_spread_ = json_get_int_attribute(specific_config, "max_spread");

        if (max_spread_ <= 0)
        {
            std::string msg = "Attribute ‘max_spread’ must be greater than 0, got "
                              + std::to_string(max_spread_);

            throw std::runtime_error(msg.c_str());
        }

        if (json_exist_attribute(specific_config, "persistent"))
        {
            if (json_get_bool_attribute(specific_config, "persistent"))
            {
                hs_.persistent();

                hft_log(INFO) << "init: Handler is persistent";
            }
        }

        tau_ = json_get_int_attribute(specific_config, "tau");

        if (tau_ <= 0)
        {
            std::string msg = "Attribute ‘tau’ must be greater than 0, got "
                              + std::to_string(tau_);

            throw std::runtime_error(msg.c_str());
        }

        trade_zone_.reset(new trade_zone(tau_));
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

void blsh::on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market)
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
        hs_.set_bool_var("position.close_attempted", false);

        hft_log(INFO) << "Synchronized LONG position ‘"
                      << position_id << "’, open price ‘"
                      << msg.price << "’";
    }

    position_confirmed_ = true;
}

void blsh::on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market)
{
    verify_position_confirmation_status();

    int ask_pips = floating2pips(msg.ask);
    int bid_pips = floating2pips(msg.bid);
    int spread = ask_pips - bid_pips;

    if (spread > max_spread_)
    {
        return;
    }

    auto as = hs_.create_autosaver();

    state position_state = (state) hs_.get_int_var("position.state");

    trade_zone_ -> tick(ask_pips, bid_pips, msg.request_time);
    auto z = trade_zone_ -> get_zone(ask_pips, bid_pips);

    switch (position_state)
    {
        case state::IDLE:
        {
            if (z == zone::LZ)
            {
                hs_.set_int_var("position.state", (int) state::TRY_OPEN_LONG);
                hs_.set_int_var("position.reference_price_pips", ask_pips);
                hs_.set_string_var("position.id", uid());
                hs_.set_bool_var("position.close_attempted", false);

                market.open_long(hs_.get_string_var("position.id"), contracts_);

                hft_log(INFO) << "(idle) Trying to take position LONG with num of contracts ‘"
                              << contracts_ << "’, ID is ‘" << hs_.get_string_var("position.id")
                              << "’, equity now is ‘" << msg.equity << "’, expected open price is ‘"
                              << msg.ask << "’, request time is ‘"
                              << boost::posix_time::to_simple_string(msg.request_time) << "’.";
            }

            break;
        }
        case state::LONG:
        {
            if (hs_.get_bool_var("position.close_attempted"))
            {
                hft_log(WARNING) << "(LONG) Reattempting close position ‘"
                                 << hs_.get_string_var("position.id") << "’.";

                hs_.set_int_var("position.state", (int) state::TRY_CLOSE_LONG);

                market.close_position(hs_.get_string_var("position.id"));

               return;
            }

            if (z == zone::HZ || z == zone::CPZ)
            {
                hft_log(INFO) << "(LONG) Going to close position ‘"
                              << hs_.get_string_var("position.id") << "’.";

                hs_.set_int_var("position.state", (int) state::TRY_CLOSE_LONG);

                market.close_position(hs_.get_string_var("position.id"));
            }

            break;
        }
        case state::TRY_OPEN_LONG:
        case state::TRY_CLOSE_LONG:
            await_position_status();
    }
}

void blsh::on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market)
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

            default:
                hft_log(ERROR) << "Unexpected position open notify";
        }
    }
    else // msg.status
    {
        switch (position_state)
        {
            case state::TRY_OPEN_LONG:
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

void blsh::on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market)
{
    auto as = hs_.create_autosaver();

    state position_state = (state) hs_.get_int_var("position.state");

    if (msg.status)
    {
        switch (position_state)
        {
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

std::string blsh::state2state_str(state position_state)
{
    switch ((state) position_state)
    {
        case state::IDLE:
            return "IDLE";
        case state::TRY_OPEN_LONG:
            return "TRY_OPEN_LONG";
        case state::LONG:
            return "LONG";
        case state::TRY_CLOSE_LONG:
            return "TRY_CLOSE_LONG";
    }

    return "???";
}

void blsh::verify_position_confirmation_status(void)
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

void blsh::await_position_status(void)
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
    }
}

} /* namespace hft_ih_plugin */
