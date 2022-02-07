#include <boost/dll.hpp>
#include <instrument_handler.hpp>
#include <virtual_player.hpp>

#ifndef __SMART_MARTINGALE_HPP__
#define __SMART_MARTINGALE_HPP__

#define API extern "C" BOOST_SYMBOL_EXPORT

namespace hft_ih_plugin {

class smart_martingale : public instrument_handler
{
public:

    smart_martingale(const instrument_handler::init_info &general_config);
    smart_martingale(void) = delete;

    ~smart_martingale(void) = default;

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

    void continue_long_position(int bid_pips, hft::protocol::response &market);
    void continue_short_position(int ask_pips, hft::protocol::response &market);

    int compute_initial_num_of_contracts(double free_margin) const;
    int compute_current_num_of_contracts(double free_margin) const;

    double get_current_money_lost(int pips_lost) const;
    double get_extra_contracts(void) const;

    //
    // Configurable parameters.
    //

    double microlot_margin_requirement_;
    double microlot_pip_value_;
    double microlot_value_commission_;
    int trade_pips_limit_;
    int max_martingale_depth_;
    int max_spread_;
    double contracts_a_;
    double contracts_b_;

    virtual_player vplayer_;
};

} /* namespace hft_ih_plugin */

API instrument_handler_ptr create_plugin(const instrument_handler::init_info &general_config)
{
    return new hft_ih_plugin::smart_martingale(general_config);
}

#endif /* __SMART_MARTINGALE_HPP__ */
