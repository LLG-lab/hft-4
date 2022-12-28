#include <boost/dll.hpp>
#include <instrument_handler.hpp>
#include <gcell.hpp>

#ifndef __GRID_HPP__
#define __GRID_HPP__

#define API extern "C" BOOST_SYMBOL_EXPORT

namespace hft_ih_plugin {

class grid : public instrument_handler
{
public:

    grid(const instrument_handler::init_info &general_config);
    grid(void) = delete;

    ~grid(void) = default;

    virtual void init_handler(const boost::json::object &specific_config);

    virtual void on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market);
    virtual void on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market);
    virtual void on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market);
    virtual void on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market);

private:

    void load_grid(void);
    void save_grid(void);

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
    int active_gcells_limit_;

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
    return new hft_ih_plugin::grid(general_config);
}

#endif /* __GRID_HPP__ */
