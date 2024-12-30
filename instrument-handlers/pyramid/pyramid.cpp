#include <pyramid.hpp>
#include <utilities.hpp>

#include <limits>
#include <cctype>

#include <boost/lexical_cast.hpp>

#include <easylogging++.h>

#define hft_log(__X__) \
    CLOG(__X__, get_logger_id().c_str())

namespace hft_ih_plugin {

pyramid::pyramid(const instrument_handler::init_info &general_config)
    : instrument_handler(general_config),
      current_state_ {state::OPERATIONAL},
      persistent_ {false},
      contracts_ {0.0},
      max_spread_ {0xFFFF},
      active_gcells_ {0},
      pyramid_height_ {2},
      dayswap_pips_ {0.0},
      positions_confirmed_ {false},
      liquidate_pyramid_ {false},
      awaiting_position_status_counter_ {false}
{
    el::Loggers::getLogger(get_logger_id().c_str(), true);

    hft_log(INFO) << "Starting instrument handler ‘Pyramid Grid™’ for instrument ‘"
                  << get_ticker() << "’, i.e. ‘"
                  << get_instrument_description()
                  << "’";
}

void pyramid::init_handler(const boost::json::object &specific_config)
{
    //
    // Going to obtain following attributes (example):
    //

    //
    //  "handler_options": {
    //      "persistent": false,     /* Optional, default: false */
    //      "contracts": 0.01,
    //      "max_spread": 12,
    //      "pyramid_height": 10,
    //      "dayswap_pips":-0.3,
    //      "grid_definition": {
    //           .
    //           .
    //           .
    //      }
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

        dayswap_pips_ = json_get_double_attribute(specific_config, "dayswap_pips");

        if (json_exist_attribute(specific_config, "pyramid_height"))
        {
            pyramid_height_ = json_get_int_attribute(specific_config, "pyramid_height");

            if (pyramid_height_ <= 1)
            {
                std::string msg = "Attribute ‘pyramid_height’ must be greater than 1, got "
                                  + std::to_string(pyramid_height_);

                throw std::runtime_error(msg.c_str());
            }
        }

        hft_log(INFO) << "init: Pyramid height are "
                      << pyramid_height_ << ".";

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

        for (auto it = gcells_.begin(); it != gcells_.end(); it++)
        {
            if (! it -> is_terminal())
            {
                hft_log(INFO) << "init: Trades from " << pips2floating(it -> get_min_limit());

                break;
            }
        }

        for (auto it = gcells_.rbegin(); it != gcells_.rend(); it++)
        {
            if (! it -> is_terminal())
            {
                hft_log(INFO) << "init: Trades to " << pips2floating(it -> get_max_limit());

                break;
            }
        }
/*
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
*/
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

void pyramid::on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market)
{
    for (int i = 0; i < gcells_.size(); i++)
    {
        if (gcells_[i].has_position() && gcells_[i].get_position_id() == msg.id)
        {
            gcells_[i].confirm_position();

            return;
        }
    }

    hft_log(WARNING) << "sync: Unrecognized position: ‘"
                     << msg.id << "’.";
}

void pyramid::on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market)
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
    // We're in trading zone.
    //

    if (liquidate_pyramid_)
    {
        //
        // Attempt to liquidate pyramid of all gcells from 0 to index - 1.
        //

        for (int i = 0; i < index; i++)
        {
            if (gcells_[i].has_position() && profitable(i, bid_pips, msg.request_time))
            {
                // Close position.

                market.close_position(gcells_[i].get_position_id());

                hft_log(INFO) << "Closing position ‘" << gcells_[i].get_position_id()
                              << "’ from cell #" << gcells_[i].get_id();

                current_state_ = state::WAIT_FOR_STATUS;

                return;
            }
        }

        liquidate_pyramid_ = false;
    }

    if (gcells_[index].is_terminal())
    {
        liquidate_pyramid_ = true;
        return;
    }

    //
    // We have four separate conditions:
    //
    // index+n |     |     |  •  |  •  |
    // --------------+------------------
    //   index |     |  •  |     |  •  |
    //           (I)  (II)  (III) (IV)
    //
    // Condition (I)   – Open position LONG on grid at index; return.
    // Condition (II)  – Try liquidate pyramid.
    // Condition (III) – Close position at index+n if such position
    //                   is lossy enough; return.
    // Condition (IV)  – Close position at index+n if such position
    //                   is lossy enough; return.
    //

    int successor_index = get_successor_position_index(index);

    // Conditions (III) / (IV).
    if (successor_index > 0 && lossy_enough(successor_index, bid_pips, msg.request_time))
    {
        // Close position.
        market.close_position(gcells_[successor_index].get_position_id());

        hft_log(INFO) << "Closing position ‘" << gcells_[successor_index].get_position_id()
                      << "’ from cell #" << gcells_[successor_index].get_id();

        current_state_ = state::WAIT_FOR_STATUS;

        return;
    }


    // Conditions (I).
    if (! gcells_[index].has_position()) // && ! gcells_[index].is_terminal())
    {
        auto pos_id = uid();
        market.open_long(pos_id, contracts_);
        gcells_[index].attach_position(pos_id, hft::utils::ptime2timestamp(msg.request_time), active_gcells_);

        hft_log(INFO) << "Opening position ‘"
                      << pos_id << "’ in cell #"
                      << gcells_[index].get_id();

        current_state_ = state::WAIT_FOR_STATUS;
    }
    else // Conditions (II).
    {
        // try liquidate pyramid.
        int n = 0;

        for (int i = 0; i < index - 1; i++)
        {
            if (gcells_[i].has_position() && profitable(i, bid_pips, msg.request_time))
            {
                n++;
            }
        }

        if (n >= pyramid_height_)
        {
            hft_log(INFO) << "Will now liquidate pyramid with n="
                          << n << " positions.";

            liquidate_pyramid_ = true;
            return;
        }
    }
}

void pyramid::on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market)
{
    if (current_state_ != state::WAIT_FOR_STATUS)
    {
        hft_log(ERROR) << "position_open: Unexpected position open notify";
    }

    for (int i = 0; i < gcells_.size(); i++)
    {
        if (gcells_[i].has_position() && gcells_[i].get_position_id() == msg.id)
        {
            if (msg.status)
            {
                gcells_[i].confirm_position(floating2pips(msg.price));

                hft_log(INFO) << "position_open: Position ‘" << msg.id
                              << "’ successfuly opened, price was "
                              << msg.price;

                save_grid();
            }
            else
            {
                gcells_[i].detatch_position(active_gcells_);

                hft_log(INFO) << "position_open: Failed to open position ‘"
                              << msg.id << "’.";
            }

            break;
        }
    }

    current_state_ = state::OPERATIONAL;
    awaiting_position_status_counter_ = 0;
}

void pyramid::on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market)
{
    if (current_state_ != state::WAIT_FOR_STATUS)
    {
        hft_log(ERROR) << "position_close: Unexpected position close notify";
    }

    if (msg.status)
    {
        for (int i = 0; i < gcells_.size(); i++)
        {
            if (gcells_[i].has_position() && gcells_[i].get_position_id() == msg.id)
            {
                gcells_[i].detatch_position(active_gcells_);

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

bool pyramid::profitable(int cell_index, int bid_pips, boost::posix_time::ptime current_time) const
{
    int days_elapsed = (current_time.date() - hft::utils::timestamp2ptime(gcells_[cell_index].get_position_timestamp()).date()).days();

    int yield = bid_pips - gcells_[cell_index].get_position_price_pips() + dayswap_pips_*days_elapsed;

    if (yield >= gcells_[cell_index].span())
    {
        return true;
    }

    return false;
}

bool pyramid::lossy_enough(int cell_index, int bid_pips, boost::posix_time::ptime current_time) const
{
    int days_elapsed = (current_time.date() - hft::utils::timestamp2ptime(gcells_[cell_index].get_position_timestamp()).date()).days();

    int yield = bid_pips - gcells_[cell_index].get_position_price_pips() + dayswap_pips_*days_elapsed;

    if ((-1)*yield >= gcells_[cell_index].span())
    {
        return true;
    }

    return false;
}

int pyramid::get_precedessor_position_index(int index) const
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

int pyramid::get_successor_position_index(int index) const
{
    for (int i = index + 1; i < gcells_.size(); i++)
    {
        if (gcells_[i].has_position())
        {
            return i;
        }
    }

    return -1;
}

std::map<char, std::pair<int, bool>> pyramid::get_cell_types(const boost::json::object &obj) const
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

void pyramid::create_grid(const boost::json::object &grid_def)
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
        gnumber++;

        gcells_.emplace_back(start_price_pips, end_price_pips, gnumber, terminal);

        start_price_pips = end_price_pips;
    }

    if (gcells_.empty())
    {
        throw std::runtime_error("Grid is empty");
    }
}

void pyramid::load_grid(void)
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

    std::string position_id;
    unsigned long position_timestamp;
    int position_price_pips;

    for (int i = 0; i < gcells_.size(); i++)
    {
        const object &position_info_obj = json_get_object_attribute(obj, gcells_[i].get_id());

        position_id = json_get_string_attribute(position_info_obj, "id");

        if (position_id.length() > 0)
        {
            position_timestamp  = boost::lexical_cast<unsigned long>(json_get_string_attribute(position_info_obj, "time"));
            position_price_pips = json_get_int_attribute(position_info_obj, "price_pips");

            hft_log(INFO) << "load grid: Attaching position ‘"
                          << position_id << "’ to cell #"
                          << gcells_[i].get_id() << " (price pips: "
                          << position_price_pips << ", opened: "
                          << hft::utils::timestamp2string(position_timestamp)
                          << ").";

            gcells_[i].attach_position(position_id, position_timestamp, position_price_pips, active_gcells_);
        }
    }
}

void pyramid::save_grid(void)
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

        //
        // {
        //     "g189":{"id":"hft_10123456a","time":1111111,"price_pips":87654},
        //     "g190":{"id":"hft_10234567b","time":2222222,"price_pips":76543}
        // }
        //

        json_data << "\t\"" << gcells_[i].get_id() << "\":{\"id\":\""
                  << gcells_[i].get_position_id() << "\",\"time\":\""
                  << gcells_[i].get_position_timestamp() << "\",\"price_pips\":"
                  << gcells_[i].get_position_price_pips() << "}";
    }

    json_data << "\n}\n";

    file_put_contents("grid.json", json_data.str());
}

void pyramid::verify_position_confirmation_status(void)
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
            hft_log(WARNING) << "Position ‘" << gcells_[i].get_position_id()
                             << "’ does not exist on market anymore, removing from grid.";

            gcells_[i].detatch_position(active_gcells_);

            to_be_save = true;
        }
    }

    if (to_be_save)
    {
        save_grid();
    }

    positions_confirmed_ = true;
}

void pyramid::await_position_status(void)
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
                    hft_log(WARNING) << "Position ‘" << gcells_[i].get_position_id()
                                     << "’ has not been confirmed within defined time, removing from grid.";

                    gcells_[i].detatch_position(active_gcells_);
                }
            }

            awaiting_position_status_counter_ = 0;
            current_state_ = state::OPERATIONAL;
        }
    }
}

} /* namespace hft_ih_plugin */
