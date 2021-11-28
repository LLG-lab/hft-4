#include <smart_martingale.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, get_logger_id().c_str())

namespace hft_ih_plugin {

smart_martingale::smart_martingale(const instrument_handler::init_info &general_config)
    : instrument_handler(general_config),
      microlot_margin_requirement_(0.0),
      microlot_pip_value_(0.0),
      microlot_value_commission_(0.0),
      trade_pips_limit_(0),
      max_martingale_depth_(1),
      num_of_contracts_(1000),
      vplayer_(hs_, "vplayer", get_logger_id())
{
    el::Loggers::getLogger(get_logger_id().c_str(), true);

    hft_log(INFO) << "Starting instrument handler ‘Smart Martingale™’ for instrument ‘"
                  << get_ticker() << "’, i.e. ‘"
                  << get_instrument_description()
                  << "’";
    //
    // Handler state variables.
    //

    hs_.set_int_var("adv", (int) virtual_player::advice::DO_NOTHING);
    hs_.set_int_var("state", (int) state::IDLE);
    hs_.set_int_var("martingale_depth", 0);
    hs_.set_int_var("initial_num_of_contracts", 0);
    hs_.set_int_var("current_num_of_contracts", 0);
    hs_.set_double_var("money_lost", 0.0);
    hs_.set_bool_var("long_ban", false);
    hs_.set_bool_var("short_ban", false);
    hs_.set_int_var("position.open_price_pips", 0);
    hs_.set_bool_var("position.close_attempted", false);
    hs_.set_string_var("position.id", "");
}

//
// Public interface.
//

void smart_martingale::init_handler(const boost::json::object &specific_config)
{
    //
    // Going to obtain following attributes (example):
    //

    //
    // "handler_options": {
    //    "microlot_margin_requirement":155.0,
    //    "microlot_pip_value":0.4,
    //    "microlot_value_commission":0.17,
    //    "trade_pips_limit":10,
    //    "max_martingale_depth":3,
    //    "num_of_contracts":1000,
    //    "persistent_state":true // OPTIONAL ATTRIBUTE
    // }
    //

    try
    {
        microlot_margin_requirement_ = json_get_double_attribute(specific_config, "microlot_margin_requirement");
        microlot_pip_value_ = json_get_double_attribute(specific_config, "microlot_pip_value");
        microlot_value_commission_ = json_get_double_attribute(specific_config, "microlot_value_commission");
        trade_pips_limit_ = json_get_int_attribute(specific_config, "trade_pips_limit");
        max_martingale_depth_ = json_get_int_attribute(specific_config, "max_martingale_depth");
        num_of_contracts_ = json_get_int_attribute(specific_config, "num_of_contracts");

        if (microlot_margin_requirement_ <= 0.0)
        {
            std::string msg = "Attribute ‘microlot_margin_requirement’ must be greater than zero, got "
                              + std::to_string(microlot_margin_requirement_);

            throw std::runtime_error(msg.c_str());
        }

        if (microlot_pip_value_ <= 0.0)
        {
            std::string msg = "Attribute ‘microlot_pip_value’ must be greater than zero, got "
                              + std::to_string(microlot_pip_value_);

            throw std::runtime_error(msg.c_str());
        }

        if (microlot_value_commission_ <= 0.0)
        {
            std::string msg = "Attribute ‘microlot_value_commission’ must be greater than zero, got "
                              + std::to_string(microlot_value_commission_);

            throw std::runtime_error(msg.c_str());
        }

        if (trade_pips_limit_ <= 0)
        {
            std::string msg = "Attribute ‘trade_pips_limit’ must be greater than zero, got "
                              + std::to_string(trade_pips_limit_);

            throw std::runtime_error(msg.c_str());
        }

        if (max_martingale_depth_ <= 0)
        {
            std::string msg = "Attribute ‘max_martingale_depth’ must be greater than zero, got "
                              + std::to_string(max_martingale_depth_);

            throw std::runtime_error(msg.c_str());
        }

        if (num_of_contracts_ < 1000)
        {
            std::string msg = "Attribute ‘num_of_contracts’ must be greater than 999, got "
                              + std::to_string(num_of_contracts_);

            throw std::runtime_error(msg.c_str());
        }

        if (json_exist_attribute(specific_config, "persistent_state") &&
                json_get_bool_attribute(specific_config, "persistent_state"))
        {
            hft_log(WARNING) << "Option ‘persistent_state’ has been set for handler. This setting may reduce performance.";

            hs_.persistent();
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

    vplayer_.set_pips_limit(trade_pips_limit_);

    hft_log(INFO) << "init: microlot_margin_requirement: ‘"
                  << microlot_margin_requirement_ << "’.";
    hft_log(INFO) << "init: microlot_pip_value: ‘"
                  << microlot_pip_value_ << "’.";
    hft_log(INFO) << "init: microlot_value_commission: ‘"
                  << microlot_value_commission_ << "’.";
    hft_log(INFO) << "init: trade_pips_limit: ‘"
                  << trade_pips_limit_ << "’.";
    hft_log(INFO) << "init: max_martingale_depth: ‘"
                  << max_martingale_depth_ << "’.";
    hft_log(INFO) << "init: num_of_contracts: ‘"
                  << num_of_contracts_ << "’.";
}

void smart_martingale::on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market)
{
    //
    // Unused in Smart Martingale handler.
    //
}

void smart_martingale::on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market)
{
    auto as = hs_.create_autosaver();

    int ask_pips = floating2pips(msg.ask);
    int bid_pips = floating2pips(msg.bid);

    vplayer_.new_market_data(ask_pips, bid_pips);

    switch ((state) hs_.get_int_var("state"))
    {
        case state::IDLE:
        {
            if (msg.equity < microlot_margin_requirement_)
            {
                hft_log(ERROR) << "You are broke !!!";

                break;
            }

            if (hs_.get_int_var("martingale_depth") == 0)
            {
                hs_.set_int_var("adv", (int) vplayer_.give_advice());

                if ((virtual_player::advice) hs_.get_int_var("adv") == virtual_player::advice::DO_NOTHING)
                {
                    break;
                }

                int contracts = compute_num_of_contracts(msg.equity);

                hs_.set_int_var("martingale_depth", 1);
                hs_.set_double_var("money_lost", 0.0);
                hs_.set_int_var("initial_num_of_contracts", contracts);
                hs_.set_int_var("current_num_of_contracts", contracts);
            }
            else
            {
                hs_.set_int_var("martingale_depth", hs_.get_int_var("martingale_depth") + 1);
                hs_.set_int_var("current_num_of_contracts", hs_.get_int_var("initial_num_of_contracts") + get_extra_contracts());
            }

            if ((virtual_player::advice) hs_.get_int_var("adv") == virtual_player::advice::PLAY_LONG)
            {
                if (hs_.get_bool_var("long_ban"))
                {
                    hs_.set_int_var("martingale_depth", 0);

                    break;
                }

                hs_.set_bool_var("short_ban", false);
                hs_.set_int_var("state", (int) state::TRY_OPEN_LONG);
                hs_.set_int_var("position.open_price_pips", ask_pips);
                hs_.set_string_var("position.id", uid());
                hs_.set_bool_var("position.close_attempted", false);

                market.open_long(hs_.get_string_var("position.id"), hs_.get_int_var("current_num_of_contracts"));

                hft_log(INFO) << "(idle) Trying to take position LONG with num of contracts ‘"
                              << hs_.get_int_var("current_num_of_contracts") << ", ID is ‘" << hs_.get_string_var("position.id")
                              << "’, equity now is ‘" << msg.equity << "’, expected open price is ‘"
                              << msg.ask << "’.";
            }
            else if ((virtual_player::advice) hs_.get_int_var("adv") == virtual_player::advice::PLAY_SHORT)
            {
                if (hs_.get_bool_var("short_ban"))
                {
                    hs_.set_int_var("martingale_depth", 0);

                    break;
                }

                hs_.set_bool_var("long_ban", false);
                hs_.set_int_var("state", (int) state::TRY_OPEN_SHORT);
                hs_.set_int_var("position.open_price_pips", bid_pips);
                hs_.set_string_var("position.id", uid());
                hs_.set_bool_var("position.close_attempted", false);

                market.open_short(hs_.get_string_var("position.id"), hs_.get_int_var("current_num_of_contracts"));

                hft_log(INFO) << "(idle) Trying to take position SHORT with num of contracts ‘"
                              << hs_.get_int_var("current_num_of_contracts") << ", ID is ‘" << hs_.get_string_var("position.id")
                              << "’, equity now is ‘" << msg.equity << "’, expected open price is ‘"
                              << msg.bid << "’.";
            }

            break;
        }
        case state::LONG:
        {
            continue_long_position(bid_pips, market);
            break;
        }
        case state::SHORT:
        {
            continue_short_position(ask_pips, market);
            break;
        }
        case state::TRY_OPEN_LONG:
        {
            hft_log(INFO) << "(try open long) Awaiting for position status ‘"
                          << hs_.get_string_var("position.id") << "’.";
            break;
        }
        case state::TRY_OPEN_SHORT:
        {
            hft_log(INFO) << "(try open short) Awaiting for position status ‘"
                          << hs_.get_string_var("position.id") << "’.";
            break;
        }
    }
}

void smart_martingale::on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market)
{
    auto as = hs_.create_autosaver();

    if (msg.status)
    {
        switch ((state) hs_.get_int_var("state"))
        {
            case state::TRY_OPEN_LONG:
                if (msg.id == hs_.get_string_var("position.id"))
                {
                    hs_.set_int_var("state", (int) state::LONG);
                    hs_.set_int_var("position.open_price_pips", floating2pips(msg.price));

                    hft_log(INFO) << "(LONG) Successfuly opened position, open price was ‘"
                                  << msg.price << "’.";
                }
                else
                {
                    std::string err = std::string("position open: Position ID missmatch: expected ‘")
                                      + hs_.get_string_var("position.id") + std::string("’, got ‘")
                                      + msg.id + std::string("’");

                    throw std::runtime_error(err);
                }

                break;
            case state::TRY_OPEN_SHORT:
                if (msg.id == hs_.get_string_var("position.id"))
                {
                    hs_.set_int_var("state", (int) state::SHORT);
                    hs_.set_int_var("position.open_price_pips", floating2pips(msg.price));

                    hft_log(INFO) << "(SHORT) Successfuly opened position, open price was ‘"
                                  << msg.price << "’.";
                }
                else
                {
                    std::string err = std::string("position open: Position ID missmatch: expected ‘")
                                      + hs_.get_string_var("position.id") + std::string("’, got ‘")
                                      + msg.id + std::string("’");

                    throw std::runtime_error(err);
                }

                break;
            default:
                throw std::runtime_error("Unexpected position open notify");
        }
    }
    else
    {
        switch ((state) hs_.get_int_var("state"))
        {
            case state::TRY_OPEN_LONG:
            case state::TRY_OPEN_SHORT:
                if (msg.id == hs_.get_string_var("position.id"))
                {
                    hs_.set_int_var("state", (int) state::IDLE);
                    hs_.set_int_var("position.open_price_pips", 0);
                    hs_.set_string_var("position.id", "");

                    hft_log(INFO) << "(idle) Failed to open position.";
                }
                else
                {
                    std::string err = std::string("position open: Position ID missmatch: expected ‘")
                                      + hs_.get_string_var("position.id") + std::string("’, got ‘")
                                      + msg.id + std::string("’");

                    throw std::runtime_error(err);
                }

                break;
            default:
                throw std::runtime_error("Unexpected position open notify");
        }
    }
}

void smart_martingale::on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market)
{
    auto as = hs_.create_autosaver();

    if (msg.status)
    {
        switch ((state) hs_.get_int_var("state"))
        {
            case state::TRY_CLOSE_SHORT:
            case state::TRY_CLOSE_LONG:
                if (msg.id == hs_.get_string_var("position.id"))
                {
                    hs_.set_int_var("state", (int) state::IDLE);
                    hs_.set_int_var("position.open_price_pips", 0);
                    hs_.set_string_var("position.id", "");

                    hft_log(INFO) << "(idle) Position ‘" << msg.id
                                  << " closed successfuly.";
                }
                else
                {
                    std::string err = std::string("position close: Position ID missmatch: expected ‘")
                                      + hs_.get_string_var("position.id") + std::string("’, got ‘")
                                      + msg.id + std::string("’");

                    throw std::runtime_error(err);
                }

                break;
            default:
                throw std::runtime_error("Unexpected position close notify");
        }
    }
    else
    {
        switch ((state) hs_.get_int_var("state"))
        {
            case state::TRY_CLOSE_SHORT:
                if (msg.id == hs_.get_string_var("position.id"))
                {
                    hs_.set_int_var("state", (int) state::SHORT);
                    hs_.set_bool_var("position.close_attempted", true);

                    hft_log(INFO) << "(SHORT) Failed to close position ‘"
                                  << hs_.get_string_var("position.id") << "’.";
                }
                else
                {
                    std::string err = std::string("position close: Position ID missmatch: expected ‘")
                                      + hs_.get_string_var("position.id") + std::string("’, got ‘")
                                      + msg.id + std::string("’");

                    throw std::runtime_error(err);
                }

                break;
            case state::TRY_CLOSE_LONG:
                if (msg.id == hs_.get_string_var("position.id"))
                {
                    hs_.set_int_var("state", (int) state::LONG);
                    hs_.set_bool_var("position.close_attempted", true);

                    hft_log(INFO) << "(LONG) Failed to close position ‘"
                                  << hs_.get_string_var("position.id") << "’.";
                }
                else
                {
                    std::string err = std::string("position close: Position ID missmatch: expected ‘")
                                      + hs_.get_string_var("position.id") + std::string("’, got ‘")
                                      + msg.id + std::string("’");

                    throw std::runtime_error(err);
                }

                break;
            default:
                throw std::runtime_error("Unexpected position close notify");
        }
    }
}

//
// Private methods.
//

void smart_martingale::continue_long_position(int bid_pips, hft::protocol::response &market)
{
    if (bid_pips - hs_.get_int_var("position.open_price_pips") >= trade_pips_limit_)
    {
         //
         // Profitable close.
         //

         hs_.set_int_var("martingale_depth", 0);
         hs_.set_int_var("state", (int) state::TRY_CLOSE_LONG);

         market.close_position(hs_.get_string_var("position.id"));

         return;
    }
    else if (hs_.get_int_var("position.open_price_pips") - bid_pips >= trade_pips_limit_)
    {
         int pips_lost = hs_.get_int_var("position.open_price_pips") - bid_pips + 4;

         if (! hs_.get_bool_var("position.close_attempted"))
         {
             hs_.set_double_var("money_lost", hs_.get_double_var("money_lost") + get_current_money_lost(pips_lost));
         }
         else
         {
             hft_log(WARNING) << "(SHORT) Reattempting close position ‘"
                              << hs_.get_string_var("position.id") << "’.";
         }

         if (hs_.get_int_var("martingale_depth") == max_martingale_depth_)
         {
             hs_.set_int_var("martingale_depth", 0);
             hs_.set_bool_var("long_ban", true);
         }

         market.close_position(hs_.get_string_var("position.id"));

         hs_.set_int_var("state", (int) state::TRY_CLOSE_LONG);
    }
}

void smart_martingale::continue_short_position(int ask_pips, hft::protocol::response &market)
{
    if (hs_.get_int_var("position.open_price_pips") - ask_pips >= trade_pips_limit_)
    {
         //
         // Profitable close.
         //

         hs_.set_int_var("martingale_depth", 0);
         hs_.set_int_var("state", (int) state::TRY_CLOSE_SHORT);
         
         market.close_position(hs_.get_string_var("position.id"));

         return;
    }
    else if (ask_pips - hs_.get_int_var("position.open_price_pips") >= trade_pips_limit_)
    {
         int pips_lost = ask_pips - hs_.get_int_var("position.open_price_pips") + 4;

         if (! hs_.get_bool_var("position.close_attempted"))
         {
             hs_.set_double_var("money_lost", hs_.get_double_var("money_lost") + get_current_money_lost(pips_lost));
         }
         else
         {
             hft_log(WARNING) << "(LONG) Reattempting close position ‘"
                              << hs_.get_string_var("position.id") << "’.";
         }

         if (hs_.get_int_var("martingale_depth") == max_martingale_depth_)
         {
             hs_.set_int_var("martingale_depth", 0);
             hs_.set_bool_var("short_ban", true);
         }

         market.close_position(hs_.get_string_var("position.id"));

         hs_.set_int_var("state", (int) state::TRY_CLOSE_SHORT);
    }
}

int smart_martingale::compute_num_of_contracts(double equity) const
{
    double money_engaged = (microlot_margin_requirement_ / 1000.0) * num_of_contracts_;

    if (money_engaged > equity)
    {
        hft_log(WARNING) << "Funds are ‘" << (equity - money_engaged )
                         << "’ underlimited. Top up your account balance! Emergencyly set the number of contracts to 1000.";

        return 1000;
    }

    return num_of_contracts_;
}

double smart_martingale::get_current_money_lost(int pips_lost) const
{
    double Q = hs_.get_int_var("current_num_of_contracts");
    double c = microlot_value_commission_;
    double p = microlot_pip_value_;

    return (Q*(pips_lost*p+2.0*c))/1000.0;
}

double smart_martingale::get_extra_contracts(void) const
{
    double Ts = hs_.get_double_var("money_lost");
    double n = trade_pips_limit_;
    double p = microlot_pip_value_;
    double c = microlot_value_commission_;

    return ceil((1000.0*Ts)/(n*p+2.0*c));
}

} /* namespace hft_ih_plugin */
