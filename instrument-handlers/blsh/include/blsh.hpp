#include <boost/dll.hpp>
#include <instrument_handler.hpp>

#ifndef __BLSH_HPP__
#define __BLSH_HPP__

#include <trade_zone.hpp>
#include <memory>

#define API extern "C" BOOST_SYMBOL_EXPORT

namespace hft_ih_plugin {

class blsh : public instrument_handler
{
public:

    blsh(const instrument_handler::init_info &general_config);
    blsh(void) = delete;

    ~blsh(void) = default;

    virtual void init_handler(const boost::json::object &specific_config);

    virtual void on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market);
    virtual void on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market);
    virtual void on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market);
    virtual void on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market);

private:

    enum class state
    {
        IDLE,
        TRY_OPEN_LONG,
        LONG,
        TRY_CLOSE_LONG
    };

    static std::string state2state_str(state position_state);

    static std::string state2state_str(int position_state)
    {
        return state2state_str((state) position_state);
    }

    int contracts_;
    int max_spread_;
    int tau_;

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

    std::shared_ptr<trade_zone> trade_zone_;
};

} /* namespace hft_ih_plugin */

API instrument_handler_ptr create_plugin(const instrument_handler::init_info &general_config)
{
    return new hft_ih_plugin::blsh(general_config);
}

#endif /* __BLSH_HPP__ */
