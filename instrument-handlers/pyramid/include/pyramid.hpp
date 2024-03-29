#include <boost/dll.hpp>
#include <instrument_handler.hpp>
#include <gcell.hpp>

#ifndef __PYRAMID_HPP__
#define __PYRAMID_HPP__

#include <utility>
#include <map>

#define API extern "C" BOOST_SYMBOL_EXPORT

namespace hft_ih_plugin {

class pyramid : public instrument_handler
{
public:

    pyramid(const instrument_handler::init_info &general_config);
    pyramid(void) = delete;

    ~pyramid(void) = default;

    virtual void init_handler(const boost::json::object &specific_config);

    virtual void on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market);
    virtual void on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market);
    virtual void on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market);
    virtual void on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market);

private:

    std::map<char, std::pair<int, bool>> get_cell_types(const boost::json::object &obj) const;
    void create_grid(const boost::json::object &grid_def);
    void load_grid(void);
    void save_grid(void);

    bool profitable(int cell_index, int bid_pips, boost::posix_time::ptime current_time) const;
    bool lossy_enough(int cell_index, int bid_pips, boost::posix_time::ptime current_time) const;
    int  get_precedessor_position_index(int index) const;
    int  get_successor_position_index(int index) const;

    enum class state
    {
        OPERATIONAL,
        WAIT_FOR_STATUS,
    };

    state current_state_;
    bool persistent_;
    double contracts_;
    int max_spread_;
    int active_gcells_;
    int pyramid_height_;
    double dayswap_pips_;

    //
    // Flag ‘positions_confirmed_’ in constructor set to false.
    // In the ‘on_sync’ method we check if the position sent
    // by the bridge matches the one we have in gcells_.
    // If ‘on_tick’ comes and ‘positions_confirmed_’ is false,
    // so this is a first tick, we traverse gcells_ to check
    // if all positions are confirmed. If some posisiotn is
    // unconfirmed, we remove them from the gcells_. After all,
    // set ‘positions_confirmed_’ to true.
    //

    bool positions_confirmed_;
    bool liquidate_pyramid_;

    void verify_position_confirmation_status(void);

    //
    // A counter used to count ticks for the WAIT_FOR_STATUS state.
    // If the counter value exceeds the set value, and bridge
    // will not give confirmation, we restore the state
    // to OPERATIONAL.
    //

    int awaiting_position_status_counter_;

    void await_position_status(void);

    std::vector<gcell> gcells_;
};

} /* namespace hft_ih_plugin */

API instrument_handler_ptr create_plugin(const instrument_handler::init_info &general_config)
{
    return new hft_ih_plugin::pyramid(general_config);
}

#endif /* __PYRAMID_HPP__ */
