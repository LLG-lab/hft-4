#include <boost/dll.hpp>
#include <instrument_handler.hpp>

#ifndef __TREND_TRACKER_HPP__
#define __TREND_TRACKER_HPP__

#include <memory>

#include <strategic_engine.hpp>

#define API extern "C" BOOST_SYMBOL_EXPORT

namespace hft_ih_plugin {

class trend_tracker : public instrument_handler
{
public:

    trend_tracker(const instrument_handler::init_info &general_config);
    trend_tracker(void) = delete;

    ~trend_tracker(void) = default;

    virtual void init_handler(const boost::json::object &specific_config);

    virtual void on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market);
    virtual void on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market);
    virtual void on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market);
    virtual void on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market);

private:

    enum class state
    {
        IDLE,
        TRY_OPEN_SHORT,
        SHORT,
        TRY_OPEN_LONG,
        LONG,
        TRY_CLOSE_SHORT,
        TRY_CLOSE_LONG
    };

    void continue_long_position(int ask_pips, int bid_pips, hft::protocol::response &market);
    void continue_short_position(int ask_pips, int bid_pips, hft::protocol::response &market);

    static std::string state2state_str(state position_state);

    static std::string state2state_str(int position_state)
    {
        return state2state_str((state) position_state);
    }
    
    int max_spread_;
    int num_of_contracts_;

    //
    // Flag ‘position_confirmed_’ in constructor set to false.
    // In the ‘on_sync’ method we check if the position sent
    // by the bridge matches the one we have in hs_.
    // If everything is fine, set the flag to true.
    // If ‘on_tick’ comes and position_confirmed is false,
    // then we delete information in hs_ related to the position.
    //

    bool position_confirmed_;

    void verify_position_confirmation_status(void);

    //
    // A counter used to count ticks for the following states:
    // TRY_OPEN_SHORT, TRY_OPEN_LONG, TRY_CLOSE_SHORT and TRY_CLOSE_LONG.
    // If the counter value exceeds the set value, and bridge
    // will not give confirmation, we restore the state according to:
    // IDLE, IDLE, SHORT and LONG respectively.
    //

    int awaiting_position_status_counter_;

    void await_position_status(void);

    std::unique_ptr<strategic_engine> strategy_;
};

} /* namespace hft_ih_plugin */

API instrument_handler_ptr create_plugin(const instrument_handler::init_info &general_config)
{
    return new hft_ih_plugin::trend_tracker(general_config);
}

#endif /* __TREND_TRACKER_HPP__ */
