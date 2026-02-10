#include <boost/dll.hpp>
#include <instrument_handler.hpp>
#include <money_management.hpp>
#include <invest_guard.hpp>
#include <gcell.hpp>

#ifndef __XGRID_HPP__
#define __XGRID_HPP__

#include <utility>
#include <map>

#define API extern "C" BOOST_SYMBOL_EXPORT

namespace hft_ih_plugin {

class xgrid : public instrument_handler
{
public:

    xgrid(const instrument_handler::init_info &general_config);
    xgrid(void) = delete;

    ~xgrid(void) = default;

    virtual void init_handler(const boost::json::object &specific_config);

    virtual void on_sync(const hft::protocol::request::sync &msg, hft::protocol::response &market);
    virtual void on_tick(const hft::protocol::request::tick &msg, hft::protocol::response &market);
    virtual void on_position_open(const hft::protocol::request::open_notify &msg, hft::protocol::response &market);
    virtual void on_position_close(const hft::protocol::request::close_notify &msg, hft::protocol::response &market);

private:

    std::map<char, std::pair<int, bool>> get_cell_types(const boost::json::object &obj) const;
    void create_money_manager(const boost::json::object &transactions);
    void parse_instrument_details(const boost::json::object &instrument_details);

    void create_grid(const boost::json::object &grid_def);
    void create_grid_simple_defined(const boost::json::object &grid_def);
    void create_grid_full_defined(const boost::json::object &grid_def);

    void load_positions(void);
    void save_positions(void);

    void update_metrics(int bid_pips, double bankroll, boost::posix_time::ptime current_time);

    bool profitable(int cell_index, int bid_pips, boost::posix_time::ptime current_time);
    int  get_precedessor_position_index(int index) const;
    bool is_persistent(void) const { return (get_session_mode() == session_mode::PERSISTENT); }

    enum class state
    {
        OPERATIONAL,
        WAIT_FOR_STATUS,
    };

    state current_state_;
    int max_spread_;
    int active_gcells_limit_;
    bool sellout_; // If set, xgrid handler will only sell existing positions.
    double immediate_money_supply_; // Capital outside the brokerage account, ready for immediate replenishment.

    //
    // Instrument_details for metrics purposes.
    // Values are in account currency.
    //

    double pip_value_per_lot_;
    double margin_required_per_lot_;
    double commission_per_lot_;
    double long_dayswap_per_lot_;

    //
    // Stuff for alarming purpose when
    // a certain number of positions
    // have been opened.
    //

    int used_cells_alarm_;
    bool user_alarmed_;

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
    position_container positions_;
    std::shared_ptr<money_management> mmgmnt_;
    invest_guard iguard_;

    unsigned int tick_counter_;
};

} /* namespace hft_ih_plugin */

API instrument_handler_ptr create_plugin(const instrument_handler::init_info &general_config)
{
    return new hft_ih_plugin::xgrid(general_config);
}

#endif /* __XGRID_HPP__ */
