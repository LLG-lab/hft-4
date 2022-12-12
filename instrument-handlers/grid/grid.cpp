#include <grid.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, get_logger_id().c_str())

namespace hft_ih_plugin {

grid::grid(const instrument_handler::init_info &general_config)
    : instrument_handler(general_config),
      current_state_ {state::OPERATIONAL},
      persistent_ {false},
      contracts_ {0.0},
      max_spread_ {0xFFFF},
      positions_confirmed_ {false},
      awaiting_position_status_counter_ {false}
{
    el::Loggers::getLogger(get_logger_id().c_str(), true);

    hft_log(INFO) << "Starting instrument handler ‘Grid™’ for instrument ‘"
                  << get_ticker() << "’, i.e. ‘"
                  << get_instrument_description()
                  << "’";
}

void grid::init_handler(const boost::json::object &specific_config)
{
    //
    // Going to obtain following attributes (example):
    //

    //
    //  "handler_options": {
    //      "persistent": false,
    //      "contracts": 0.01,
    //      "max_spread": 12,
    //      "price_limit": 32.24,
    //      "gcells": 18
    //  }
    //

    try
    {
        if (json_exist_attribute(specific_config, "persistent"))
        {
            if (json_get_bool_attribute(specific_config, "persistent"))
            {
                persistent_ = true;

                hft_log(INFO) << "init: Handler is persistent";
            }
        }

        contracts_ = json_get_double_attribute(specific_config, "contracts");

        if (contracts_ <= 0.0)
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

        int price_limit_pips = floating2pips(json_get_double_attribute(specific_config, "price_limit"));

        if (price_limit_pips <= 0)
        {
            std::string msg = "Attribute ‘price_limit’ must be greater than 0, got "
                              + std::to_string(json_get_double_attribute(specific_config, "price_limit"));

            throw std::runtime_error(msg.c_str());
        }

        int gcells_number = json_get_int_attribute(specific_config, "gcells");

        if (gcells_number <= 0)
        {
            std::string msg = "Attribute ‘gcells’ must be greater than 0, got "
                              + std::to_string(gcells_number);

            throw std::runtime_error(msg.c_str());
        }

        int pips_span = price_limit_pips / gcells_number;

        if (pips_span < 20)
        {
            throw std::runtime_error("Too many gcells");
        }

        //
        // Create grid.
        //

        for (int i = 0; i < gcells_number - 1; i++)
        {
            gcells_.emplace_back(pips_span, i, false);
        }

        gcells_.emplace_back(100, gcells_number - 1, true);

        hft_log(INFO) << "init: Created grid with ‘" << gcells_.size()
                      << "’ gcells, pips span for each ‘"
                      << pips_span << "’.";

        load_grid();
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

void grid::on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market)
{
    for (int i = 0; i < gcells_.size(); i++)
    {
        if (gcells_[i].has_position() && gcells_[i].get_position() == msg.id)
        {
            gcells_[i].confirm_position();

            return;
        }
    }

    hft_log(WARNING) << "sync: Unrecognized position: ‘"
                     << msg.id << "’.";
}

void grid::on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market)
{
    verify_position_confirmation_status();

    if (current_state_ == state::WAIT_FOR_STATUS)
    {
        await_position_status();

        return;
    }

    int ask_pips = floating2pips(msg.ask);
    int bid_pips = floating2pips(msg.bid);
    int spread = ask_pips - bid_pips;

    if (spread > max_spread_)
    {
        return;
    }

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
    // We're in tradeing zone.
    //

    if (gcells_[index].is_terminal())
    {
        if (index > 0 && gcells_[index-1].has_position() && gcells_[index-1].has_position_confirmed())
        {
            market.close_position(gcells_[index-1].get_position());

            hft_log(INFO) << "Closing position ‘" << gcells_[index-1].get_position()
                          << "’ from terminal cell #" << gcells_[index-1].get_id();

            current_state_ = state::WAIT_FOR_STATUS;
        }

        return;
    }

    if (index == 0)
    {
        if (! gcells_[index].has_position())
        {
            auto pos_id = uid();
            market.open_long(pos_id, contracts_);
            gcells_[index].attach_position(pos_id);

            hft_log(INFO) << "Opening position ‘"
                          << pos_id << "’ in cell #"
                          << gcells_[index].get_id();

            current_state_ = state::WAIT_FOR_STATUS;
        }

        return;
    }

    //
    // We have four separate conditions:
    //
    //   index |     |     |  •  |  •  |
    // --------------+------------------
    // index–1 |     |  •  |     |  •  |
    //           (I)  (II)  (III) (IV)

    // Condition (I).
    if (! gcells_[index].has_position() && ! gcells_[index-1].has_position())
    {
        auto pos_id = uid();
        market.open_long(pos_id, contracts_);
        gcells_[index].attach_position(pos_id);

        hft_log(INFO) << "Opening position ‘"
                      << pos_id << "’ in cell #"
                      << gcells_[index].get_id();

        current_state_ = state::WAIT_FOR_STATUS;

        return;
    }

    // Condition (II).
    if (! gcells_[index].has_position() && gcells_[index-1].has_position())
    {
        auto pos_id = gcells_[index-1].get_position();
        gcells_[index-1].detatch_position(pos_id);
        gcells_[index].attach_position(pos_id);
        gcells_[index].confirm_position();

        hft_log(INFO) << "Internal transfered position ‘" << pos_id
                      << "’: #" << gcells_[index-1].get_id()
                      << " → #" << gcells_[index].get_id() << ".";

        return;
    }

    // Condition (III)
    if (gcells_[index].has_position() && ! gcells_[index-1].has_position())
    {
        // Do nothing.
        return;
    }

    // Condition (IV)
    if (gcells_[index].has_position() && gcells_[index-1].has_position())
    {
        market.close_position(gcells_[index-1].get_position());

        hft_log(INFO) << "Closing position ‘" << gcells_[index-1].get_position()
                      << "’ from cell #" << gcells_[index-1].get_id();

        current_state_ = state::WAIT_FOR_STATUS;

        return;
    }
}

void grid::on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market)
{
    if (current_state_ != state::WAIT_FOR_STATUS)
    {
        hft_log(ERROR) << "position_open: Unexpected position open notify";
    }

    for (int i = 0; i < gcells_.size(); i++)
    {
        if (gcells_[i].has_position() && gcells_[i].get_position() == msg.id)
        {
            if (msg.status)
            {
                gcells_[i].confirm_position();

                hft_log(INFO) << "position_open: Position ‘" << msg.id
                              << "’ successfuly opened, price was "
                              << msg.price;

                save_grid();
            }
            else
            {
                gcells_[i].detatch_position(msg.id);

                hft_log(INFO) << "position_open: Failed to open position ‘"
                              << msg.id << "’.";
            }

            break;
        }
    }

    current_state_ = state::OPERATIONAL;
    awaiting_position_status_counter_ = 0;
}

void grid::on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market)
{
    if (current_state_ != state::WAIT_FOR_STATUS)
    {
        hft_log(ERROR) << "position_close: Unexpected position close notify";
    }

    if (msg.status)
    {
        for (int i = 0; i < gcells_.size(); i++)
        {
            if (gcells_[i].has_position() && gcells_[i].get_position() == msg.id)
            {
                gcells_[i].detatch_position(msg.id);

                save_grid();

                hft_log(INFO) << "position_close: Closed position ‘"
                              << msg.id << "’, price: " << msg.price;

                break;
            }
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

void grid::load_grid(void)
{
    using namespace boost::json;

    if (! persistent_)
    {
        return;
    }

    std::string json_data;

    try
    {
        json_data = file_get_contents("grid.json");
    }
    catch (const std::runtime_error &e)
    {
        hft_log(WARNING) << "load grid: Unalbe to load file ‘grid.json’";

        return;
    }

    value jv;

    try
    {
        jv = parse(json_data);
    }
    catch (const system_error &e)
    {
        throw std::runtime_error("Failed to parse file ‘grid.json’");
    }

    if (jv.kind() != kind::object)
    {
        throw std::runtime_error("Invalid file ‘grid.json’");
    }

    object const &obj = jv.get_object();

    std::string pos_id;

    for (int i = 0; i < gcells_.size(); i++)
    {
        pos_id = json_get_string_attribute(obj, gcells_[i].get_id());

        if (pos_id.length() > 0)
        {
            hft_log(INFO) << "load grid: Attaching position ‘"
                          << pos_id << "’ to cell #"
                          << gcells_[i].get_id() << ".";

            gcells_[i].attach_position(pos_id);
        }
    }
}

void grid::save_grid(void)
{
    if (! persistent_)
    {
        return;
    }

    std::ostringstream json_data;

    json_data << "{\n";

    for (int i = 0; i < gcells_.size(); i++)
    {
        if (i > 0)
        {
            json_data << ",\n";
        }

        json_data << "\t\"" << gcells_[i].get_id() << "\":\""
                  << gcells_[i].get_position() << "\"";
    }

    json_data << "\n}\n";

    file_put_contents("grid.json", json_data.str());
}

void grid::verify_position_confirmation_status(void)
{
    if (positions_confirmed_)
    {
        return;
    }

    bool to_be_save = false;

    for (int i = 0; i < gcells_.size(); i++)
    {
        if (gcells_[i].has_position() && ! gcells_[i].has_position_confirmed())
        {
            hft_log(WARNING) << "Position ‘" << gcells_[i].get_position()
                             << "’ does not exist on market anymore, removing from grid.";

            gcells_[i].detatch_position(gcells_[i].get_position());

            to_be_save = true;
        }
    }

    if (to_be_save)
    {
        save_grid();
    }

    positions_confirmed_ = true;
}

void grid::await_position_status(void)
{
    static const int max_aps_counter_value = 60;

    if (current_state_ == state::WAIT_FOR_STATUS)
    {
        awaiting_position_status_counter_++;

        if (awaiting_position_status_counter_ > max_aps_counter_value)
        {
            for (int i = 0; i < gcells_.size(); i++)
            {
                if (gcells_[i].has_position() && ! gcells_[i].has_position_confirmed())
                {
                    hft_log(WARNING) << "Position ‘" << gcells_[i].get_position()
                                     << "’ has not been confirmed within defined time, removing from grid.";

                    gcells_[i].detatch_position(gcells_[i].get_position());
                }
            }

            awaiting_position_status_counter_ = 0;
            current_state_ = state::OPERATIONAL;
        }
    }
}

} /* namespace hft_ih_plugin */
